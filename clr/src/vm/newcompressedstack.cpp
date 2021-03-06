// ==++==
// 
//   
//    Copyright (c) 2006 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
#include "common.h"
#include "newcompressedstack.h"
#include "security.h"
#include "securitydescriptor.h"
#include "appdomainhelper.h"
#include "securitystackwalk.h"
#include "appdomainstack.inl"
#include "appdomain.inl"


DomainCompressedStack::DomainCompressedStack(ADID domainID)
:   m_DomainID(domainID),
    m_ignoreAD(FALSE),
    m_dwOverridesCount(0),
    m_dwAssertCount(0),
    m_Homogeneous(FALSE)
{
    WRAPPER_CONTRACT;    
}

BOOL DomainCompressedStack::IsAssemblyPresent(SharedSecurityDescriptor* ssd)
{
    CONTRACTL 
    {
        MODE_ANY;
        GC_NOTRIGGER;
        NOTHROW;
    }CONTRACTL_END;


    // Only checks the first level and does not recurse into compressed stacks
    void* pEntry = NULL;

    if (m_EntryList.GetCount() == 0)
        return FALSE;
    
    // Quick check the last entry we added - common case
    pEntry = m_EntryList.Get(m_EntryList.GetCount() - 1);
    if (pEntry == (void *)SET_LOW_BIT(ssd))
        return TRUE;

    // Go thru the whole list now - is this optimal?
    ArrayList::Iterator iter = m_EntryList.Iterate();

    while (iter.Next())
    {
        pEntry = iter.GetElement();
        if (pEntry == (void *)SET_LOW_BIT(ssd))
            return TRUE;
    }
    return FALSE;
}    

void DomainCompressedStack::AddEntry(void * ptr)
{
    CONTRACTL
    {
        THROWS;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    IfFailThrow(m_EntryList.Append(ptr));
    
}
VOID FrameSecurityDescriptorCopyFrom(FRAMESECDESCREF newFsdRef, FRAMESECDESCREF fsd)
{
    newFsdRef->SetImperativeAssertions(fsd->GetImperativeAssertions());
    newFsdRef->SetImperativeDenials(fsd->GetImperativeDenials());
    newFsdRef->SetImperativeRestrictions(fsd->GetImperativeRestrictions());
    newFsdRef->SetDeclarativeAssertions(fsd->GetDeclarativeAssertions());
    newFsdRef->SetDeclarativeDenials(fsd->GetDeclarativeDenials());
    newFsdRef->SetDeclarativeRestrictions(fsd->GetDeclarativeRestrictions());
    newFsdRef->SetAssertAllPossible(fsd->HasAssertAllPossible());
    newFsdRef->SetAssertFT(fsd->HasAssertFT());            	
}

void DomainCompressedStack::AddFrameEntry(AppDomain *pAppDomain, FRAMESECDESCREF fsdRef)
{
    CONTRACTL
    {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
    }
    CONTRACTL_END;

    ENTER_DOMAIN_PTR(pAppDomain,ADV_RUNNINGIN) //have it on the stack
    {
        struct gc
        {
            OBJECTREF fsdRef;
            OBJECTREF newFsdRef;
        } gc;
        ZeroMemory( &gc, sizeof( gc ) );
        gc.fsdRef = (OBJECTREF)fsdRef;
        
        GCPROTECT_BEGIN(gc);

        static MethodTable* pMethFrameSecDesc = NULL;
        if (pMethFrameSecDesc == NULL)
            pMethFrameSecDesc = g_Mscorlib.GetClass(CLASS__FRAME_SECURITY_DESCRIPTOR);

        gc.newFsdRef = AllocateObject(pMethFrameSecDesc);

        // We will not call the ctor and instead patch up the object based on the fsdRef passed in
        FRAMESECDESCREF newFsdRef = (FRAMESECDESCREF)gc.newFsdRef;
        FRAMESECDESCREF fsdRef1 = (FRAMESECDESCREF)gc.fsdRef;
        FrameSecurityDescriptorCopyFrom(newFsdRef, fsdRef1);
        OBJECTHANDLEHolder  tmpHnd(pAppDomain->CreateHandle(gc.newFsdRef)); 
    
        AddEntry((void*)tmpHnd);
        tmpHnd.SuppressRelease();
        GCPROTECT_END();
    
    }
    END_DOMAIN_TRANSITION;

}


void DomainCompressedStack::Destroy(void)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    // Clear Domain info (handles etc.) if the AD has not been unloaded.
    ClearDomainInfo();
    return;
}

FCIMPL1(DWORD, DomainCompressedStack::GetDescCount, DomainCompressedStack* dcs)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_COOPERATIVE;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    FCUnique(0x42);

    return dcs->m_EntryList.GetCount();
}
FCIMPLEND

FCIMPL3(void, DomainCompressedStack::GetDomainPermissionSets, DomainCompressedStack* dcs, OBJECTREF* ppGranted, OBJECTREF* ppDenied)
{
    CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;        
    }
    CONTRACTL_END;
    HELPER_METHOD_FRAME_BEGIN_0();

    *ppGranted = NULL;
    *ppDenied  = NULL;

    AppDomain* appDomain = SystemDomain::GetAppDomainFromId(dcs->GetMyDomain(),ADV_RUNNINGIN);
    if (appDomain == NULL)
    {
        // this might be the unloading AD
        AppDomain *pUnloadingDomain = SystemDomain::System()->AppDomainBeingUnloaded();
        if (pUnloadingDomain && pUnloadingDomain->GetId() == dcs->m_DomainID)
        {
#ifdef _DEBUG
            CheckADValidity(pUnloadingDomain, ADV_RUNNINGIN);
#endif
            appDomain = pUnloadingDomain;
        }
    }
    _ASSERTE(appDomain != NULL);
    if (appDomain != NULL)
    {
        ApplicationSecurityDescriptor * pAppSecDesc = appDomain->GetSecurityDescriptor();
        _ASSERTE(pAppSecDesc != NULL);
        if (pAppSecDesc != NULL)
        {
            *ppGranted = pAppSecDesc->GetGrantedPermissionSet(ppDenied);
        }
    }

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND
    
FCIMPL6(FC_BOOL_RET, DomainCompressedStack::GetDescriptorInfo, DomainCompressedStack* dcs, DWORD index, OBJECTREF* ppGranted, OBJECTREF* ppDenied, OBJECTREF* ppAssembly, OBJECTREF* ppFSD)
{
     CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;        
    }
    CONTRACTL_END;
    _ASSERTE(dcs != NULL);
    AppDomain* pCurrentDomain = GetAppDomain();
    BOOL bRetVal = FALSE;

    HELPER_METHOD_FRAME_BEGIN_RET_0()
    *ppGranted = NULL;
    *ppDenied  = NULL;
    *ppAssembly = NULL;
    *ppFSD = NULL;
    void* pEntry = dcs->m_EntryList.Get(index);
    _ASSERTE(pEntry != NULL);
    if (IS_LOW_BIT_SET(pEntry))
    {
        // Assembly found
        SharedSecurityDescriptor* pSharedSecDesc = (SharedSecurityDescriptor* )UNSET_LOW_BIT(pEntry);
        Assembly* pAssembly = pSharedSecDesc->GetAssembly();
        AssemblySecurityDescriptor* pAsmSecDesc = pAssembly->GetSecurityDescriptor( pCurrentDomain );
        *ppGranted = pAsmSecDesc->GetGrantedPermissionSet(ppDenied);
        *ppAssembly = pAssembly->GetExposedObject();
    }
    else
    {
        //FSD
        OBJECTHANDLE objHnd = (OBJECTHANDLE)pEntry;
        if (objHnd == NULL)
        {
            // throw an ADUnloaded exception which we will catch and then look at the serializedBlob
           COMPlusThrow(kAppDomainUnloadedException);
        }
        *ppFSD = ObjectFromHandle(objHnd);
        bRetVal = TRUE;
    }
    HELPER_METHOD_FRAME_END();
    
    FC_RETURN_BOOL(bRetVal);
}
FCIMPLEND

FCIMPL1(FC_BOOL_RET, DomainCompressedStack::IgnoreDomain, DomainCompressedStack* dcs)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_COOPERATIVE;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    _ASSERTE(dcs != NULL);

    if (dcs->m_ignoreAD)
        FC_RETURN_BOOL(TRUE);

    AppDomainFromIDHolder appDomain(dcs->GetMyDomain(), TRUE);
    if (!appDomain.IsUnloaded())
    {
        ApplicationSecurityDescriptor * pAppSecDesc = appDomain->GetSecurityDescriptor();
        _ASSERTE(pAppSecDesc != NULL);
        if (pAppSecDesc != NULL)
        {
            FC_RETURN_BOOL(pAppSecDesc->IsDefaultAppDomain());
        }
    }
    
    FC_RETURN_BOOL(FALSE);
}
FCIMPLEND


/*
    Note that this function is called only once: when the managed PLS is being created.
    It's possible that 2 threads could race at that point: only downside of that is that they will both do the work. No races.
    Also, we'll never be operating on a DCS whose domain is not on the current callstack. This eliminates all kinds of ADU/demand eval races.
*/
OBJECTREF DomainCompressedStack::GetDomainCompressedStackInternal(AppDomain *pDomain)
{
     CONTRACTL
    {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
    }
    CONTRACTL_END;

    AppDomain* pCurrentDomain = GetAppDomain();

    NewArrayHolder<BYTE> pbtmpSerializedObject(NULL);
    DWORD cbtmpSerializedObject = 0;

        struct gc
        {
            OBJECTREF refRetVal;
        } gc;
        ZeroMemory( &gc, sizeof( gc ) );
        
    GCPROTECT_BEGIN( gc );

    // Create object
    ENTER_DOMAIN_ID (GetMyDomain()) //on the stack
    {

        // Go ahead and create the object
        MethodDescCallSite createManagedObject(METHOD__DOMAIN_COMPRESSED_STACK__CREATE_MANAGED_OBJECT);
        ARG_SLOT args[] = {PtrToArgSlot(this)};
        gc.refRetVal = createManagedObject.Call_RetOBJECTREF(args);


                // Do we want to marshal this object also?
        if (GetAppDomain() != pCurrentDomain)
                {
                    // Serialize to a blob;
                    AppDomainHelper::MarshalObject(GetAppDomain(), &gc.refRetVal, &pbtmpSerializedObject, &cbtmpSerializedObject);
            if (pbtmpSerializedObject == NULL)
	{
            // this is an error: possibly an OOM prevented the blob from getting created.
            // We could return null and let the managed code use a fully restricted object or throw here.
            // Let's throw here...
            COMPlusThrow(kSecurityException);
        }
        }
    }
    END_DOMAIN_TRANSITION
    if (GetMyDomain() != pCurrentDomain->GetId())
        AppDomainHelper::UnmarshalObject(pCurrentDomain,pbtmpSerializedObject, cbtmpSerializedObject, &gc.refRetVal);

    GCPROTECT_END();
   
    return gc.refRetVal;
}


void DomainCompressedStack::ClearDomainInfo(void)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;


    // So, assume mutual exclusion holds and no races occur here


    // Now it is time to go NULL out any ObjectHandle we're using in the list of entries
    ArrayList::Iterator iter = m_EntryList.Iterate();

    while (iter.Next())
    {
        void* pEntry = iter.GetElement();
        if (!IS_LOW_BIT_SET(pEntry))
        {
            DestroyHandle((OBJECTHANDLE)pEntry);
        }
        pEntry = NULL;         
    }


    // Always clear the index into the domain object list and the domainID.
    m_DomainID = ADID(INVALID_APPDOMAIN_ID);    
    return;
}

NewCompressedStack::NewCompressedStack()
:     m_DCSListCount(0),
      m_currentDCS(NULL),
      m_pCtxTxFrame(NULL),
      m_CSAD(ADID(INVALID_APPDOMAIN_ID)),      
      m_ADStack(GetThread()->GetAppDomainStack()),
      m_dwOverridesCount(0),
      m_dwAssertCount(0)
{
    WRAPPER_CONTRACT;
    

    _ASSERTE(m_ADStack.GetNumDomains() > 0);
    m_ADStack.InitDomainIteration(&adStackIndex);
    m_DCSList = new DomainCompressedStack*[m_ADStack.GetNumDomains()];
    memset(m_DCSList, 0, (m_ADStack.GetNumDomains()*sizeof(DomainCompressedStack*)));
    
}


void NewCompressedStack::Destroy( CLR_BOOL bEntriesOnly )
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;
    
    if (m_DCSList != NULL)
    {
        m_currentDCS = NULL;
        m_pCtxTxFrame = NULL;
        for (DWORD i=0; i< m_ADStack.GetNumDomains(); i++)
        {
            DomainCompressedStack* dcs = m_DCSList[i];
            if (dcs != NULL)
            {
                dcs->Destroy();
                delete dcs;
            }
        }
        delete[] m_DCSList;
        m_DCSList = NULL;
    }
    if (!bEntriesOnly)
        delete this;

}


void NewCompressedStack::ProcessAppDomainTransition(void)
{
    CONTRACTL {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
    } CONTRACTL_END;

    // Get the current adstack entry. Note that the first time we enter this function, adStackIndex will 
    // equal the size of the adstack array (similar to what happens in IEnumerator).
    // So the initial pEntry will be NULL
    AppDomainStackEntry *pEntry = 
        (adStackIndex == m_ADStack.GetNumDomains() ? NULL : m_ADStack.GetCurrentDomainEntryOnStack(adStackIndex));

    // Updated the value on the ADStack for current domain
    if (pEntry != NULL)
    {
        DWORD domainOverrides_measured = (m_currentDCS == NULL?0:m_currentDCS->GetOverridesCount());
        DWORD domainAsserts_measured = (m_currentDCS == NULL?0:m_currentDCS->GetAssertCount());
        if (pEntry->m_dwOverridesCount != domainOverrides_measured || pEntry->m_dwAsserts != domainAsserts_measured)
        {
            m_ADStack.UpdateDomainOnStack(adStackIndex, domainAsserts_measured, domainOverrides_measured);
            GetThread()->UpdateDomainOnStack(adStackIndex, domainAsserts_measured, domainOverrides_measured);
        }
    }

    // Move the domain index forward if this is not the last entry
    if (adStackIndex > 0)
    m_ADStack.GetNextDomainEntryOnStack(&adStackIndex);
    m_currentDCS = NULL;

    return;
    
}
DWORD NewCompressedStack::ProcessFrame(AppDomain* pAppDomain, SharedSecurityDescriptor* pSsd, FRAMESECDESCREF* pFsdRef)
{
    // This function will be called each time we hit a new stack frame in a stack walk.

    CONTRACTL {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
        PRECONDITION(CheckPointer(pAppDomain));
        PRECONDITION(CheckPointer(pSsd));
    } CONTRACTL_END;

    // Get the current adstack entry. Note that the first time we enter this function, adStackIndex will 
    // equal the size of the adstack array (similar to what happens in IEnumerator).
    // So the initial pEntry will be NULL
    AppDomainStackEntry *pEntry = 
        (adStackIndex == m_ADStack.GetNumDomains() ? NULL : m_ADStack.GetCurrentDomainEntryOnStack(adStackIndex));


    _ASSERTE(pEntry != NULL);
    PREFIX_ASSUME(pEntry != NULL);
    FRAMESECDESCREF FsdRef = (pFsdRef!=NULL?*pFsdRef:NULL);
    DWORD dwFlags = 0;
    if (FsdRef != NULL)
    {
            
        if (FsdRef->HasAssertFT())
            dwFlags |= CORSEC_FT_ASSERT;
    }

    if ((Security::DoesFullTrustMeanFullTrust() && pEntry->IsFullyTrustedWithNoStackModifiers()) ||
        (m_currentDCS != NULL && m_currentDCS->m_Homogeneous))
    {
        // Nothing to do in this entire AD. 
        return dwFlags;
    }
    
    ADID dNewDomainID = pAppDomain->GetId();
    BOOL bAddSSD = (!pSsd->IsSystem() && !IsAssemblyPresent(dNewDomainID, pSsd));
    BOOL bHasStackModifiers = FALSE;
    DWORD overridesCount = 0;
    DWORD assertCount = 0;

    
    if (FsdRef != NULL)
    {
        overridesCount += FsdRef->GetOverridesCount();
        assertCount += FsdRef->GetAssertCount();
        bHasStackModifiers = ( (assertCount + overridesCount) > 0);
    }
    

        
    if ((bAddSSD || bHasStackModifiers ) && (m_currentDCS == NULL || m_currentDCS->m_DomainID != dNewDomainID))
    {
        CreateDCS(dNewDomainID);
    }

     // Add the SharedSecurityDescriptor (Assembly) to the list if it is not already present in the list
    if (bAddSSD)
    {
        m_currentDCS->AddEntry((void*)SET_LOW_BIT(pSsd));
        if (pEntry->IsHomogeneousWithNoStackModifiers())
            m_currentDCS->m_Homogeneous = TRUE;
    }    
    if (bHasStackModifiers)
    {
        // We need to add the FSD entry here 
        m_currentDCS->AddFrameEntry(pAppDomain, FsdRef);
        m_currentDCS->m_dwOverridesCount += overridesCount;
        m_currentDCS->m_dwAssertCount += assertCount;
        m_dwOverridesCount += overridesCount;
        m_dwAssertCount += assertCount;
    }
    return dwFlags;
}

// Build a CompressedStack given that all domains on the stack are homogeneous with no stack modifiers
FCIMPL1(void, NewCompressedStack::FCallGetHomogeneousPLS, Object* hgPLSUnsafe)
{
    CONTRACTL {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
    } CONTRACTL_END;
    
    OBJECTREF refHomogeneousPLS = (OBJECTREF)hgPLSUnsafe;

    HELPER_METHOD_FRAME_BEGIN_1(refHomogeneousPLS); 



    // Walk the adstack and update the grantSetUnion
    AppDomainStack* pADStack = GetThread()->GetAppDomainStackPointer();
    DWORD dwAppDomainIndex;
    pADStack->InitDomainIteration(&dwAppDomainIndex);
    
    while (dwAppDomainIndex != 0)
    {
        AppDomainStackEntry* pEntry = pADStack->GetNextDomainEntryOnStack(&dwAppDomainIndex);
        _ASSERTE(pEntry != NULL);
        
        pEntry->UpdateHomogeneousPLS(&refHomogeneousPLS);
    }
    
        
    HELPER_METHOD_FRAME_END();
    return ;
}
FCIMPLEND;

// Special case of ProcessFrame called with the CS at the base of the thread
void NewCompressedStack::ProcessCS(AppDomain* pAppDomain, COMPRESSEDSTACKREF csRef, Frame *pFrame)
{
    CONTRACTL {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
        PRECONDITION(CheckPointer(pAppDomain));
    } CONTRACTL_END;

    _ASSERTE(csRef != NULL && "Shouldn't call this function if CS is NULL");
    ADID dNewDomainID = pAppDomain->GetId();
    NewCompressedStack* pCS = (NewCompressedStack* )csRef->GetUnmanagedCompressedStack();
    if (csRef->IsEmptyPLS() && (pCS == NULL || pCS->GetDCSListCount() == 0))
    {
        // Do nothing - empty inner CS
        return;
    }

    // Let's special case the 1-domain CS that has no inner CSs here
    // Check for:
    // 1. == 1 DCS 
    // 2. DCS is in correct AD (it's possible that pCS is AD-X and it has only one DCS in AD-Y, because AD-X has only mscorlib frames
    // 3. No inner CS
    if ( pCS != NULL &&
         pCS->GetDCSListCount() == 1 &&
         pCS->m_DCSList != NULL &&
         pCS->m_currentDCS != NULL &&
         pCS->m_currentDCS->m_DomainID == dNewDomainID &&
         pCS->m_CSAD == ADID(INVALID_APPDOMAIN_ID))
    {
        ProcessSingleDomainNCS(pCS, pAppDomain);
    }
    else
    {

        // set flag to ignore Domain grant set if the current DCS is the same as the one with the CS
        if (m_currentDCS != NULL && m_currentDCS->m_DomainID == dNewDomainID)
        {
            m_currentDCS->m_ignoreAD = TRUE;
        }

        // Update overrides/asserts
        if (pCS != NULL)
        {
            m_dwOverridesCount += pCS->GetOverridesCount();
            m_dwAssertCount += pCS->GetAssertCount();
        }
        

        // Do we need to store the CtxTransitionFrame or did we get the CS from the thread?
        if (pFrame != NULL)
        {
            _ASSERTE(csRef == SecurityStackWalk::GetCSFromContextTransitionFrame(pFrame));
            // Use data from the CtxTxFrame
            m_pCtxTxFrame = pFrame;
        }
        m_CSAD = dNewDomainID;
    }

}
void NewCompressedStack::ProcessSingleDomainNCS(NewCompressedStack *pCS, AppDomain* pAppDomain)
{
    
    CONTRACTL {
        THROWS;
        GC_TRIGGERS;
        MODE_COOPERATIVE;
        PRECONDITION(CheckPointer(pAppDomain));
    } CONTRACTL_END;

    _ASSERTE(pCS->GetDCSListCount() <= 1 && pCS->m_CSAD == ADID(INVALID_APPDOMAIN_ID));
    ADID newDomainID = pAppDomain->GetId();
    DomainCompressedStack* otherDCS = pCS->m_currentDCS;

    if (otherDCS == NULL)
        return;
    if (m_currentDCS == NULL)
        CreateDCS(newDomainID);


    // Iterate thru the entryList in the current DCS
    ArrayList::Iterator iter = otherDCS->m_EntryList.Iterate();
    while (iter.Next())
    {
        void* pEntry = iter.GetElement();
        if (IS_LOW_BIT_SET(pEntry))
        {
            if (!IsAssemblyPresent(newDomainID, (SharedSecurityDescriptor*)UNSET_LOW_BIT(pEntry)))
            {
                //Add the assembly
                m_currentDCS->AddEntry(pEntry);
            }
        }
        else
        {
            // FrameSecurityDescriptor 
            OBJECTHANDLE objHnd = (OBJECTHANDLE)pEntry;
            OBJECTREF fsdRef = ObjectFromHandle(objHnd);
            OBJECTHANDLEHolder  tmpHnd(pAppDomain->CreateHandle(fsdRef)); 

            m_currentDCS->AddEntry((void*)tmpHnd);
            tmpHnd.SuppressRelease();
            
        }
            
    }    

    m_currentDCS->m_dwOverridesCount += pCS->m_dwOverridesCount;
    m_currentDCS->m_dwAssertCount += pCS->m_dwAssertCount;
    m_dwOverridesCount += pCS->m_dwOverridesCount;
    m_dwAssertCount += pCS->m_dwAssertCount;    
}
void NewCompressedStack::CreateDCS(ADID domainID)
{
    CONTRACTL {
        THROWS;
        GC_NOTRIGGER;
        MODE_ANY;
    } CONTRACTL_END;

    _ASSERTE(adStackIndex < m_ADStack.GetNumDomains());
    _ASSERTE (m_DCSList != NULL);
    m_DCSList[adStackIndex] = new DomainCompressedStack(domainID);
    m_currentDCS = m_DCSList[adStackIndex];
    m_DCSListCount++;

    return;
}


BOOL NewCompressedStack::IsAssemblyPresent(ADID domainID, SharedSecurityDescriptor* pSsd)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    _ASSERTE(domainID != ADID(INVALID_APPDOMAIN_ID) && "Don't pass invalid domain");

    BOOL bEntryPresent = FALSE;
    
    for(DWORD i=0; i < m_ADStack.GetNumDomains(); i++)
    {

        DomainCompressedStack* pTmpDCS = m_DCSList[i];
        
        if (pTmpDCS != NULL && pTmpDCS->m_DomainID == domainID && pTmpDCS->IsAssemblyPresent(pSsd))
        {
            bEntryPresent = TRUE;
            break;
        }
    }
    return bEntryPresent;
}


BOOL NewCompressedStack::IsDCSContained(DomainCompressedStack *pDCS)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    // return FALSE if no DCS or DCS is for a domain that has been unloaded
    if (pDCS == NULL || pDCS->m_DomainID == ADID(INVALID_APPDOMAIN_ID))
        return FALSE;
    

    
    // Iterate thru the entryList in the current DCS
    ArrayList::Iterator iter = pDCS->m_EntryList.Iterate();
    while (iter.Next())
    {
        void* pEntry = iter.GetElement();
        if (IS_LOW_BIT_SET(pEntry))
        {
            // We only check Assemblies.
            if (!IsAssemblyPresent(pDCS->m_DomainID, (SharedSecurityDescriptor*)UNSET_LOW_BIT(pEntry)))
            return FALSE;
        }    
    }    
    return TRUE;
}

BOOL NewCompressedStack::IsNCSContained(NewCompressedStack *pCS)
{
    CONTRACTL
    {
        NOTHROW;
        GC_NOTRIGGER;
        MODE_ANY;
    }
    CONTRACTL_END;

    // Check if the first level of pCS is contained in this.
    if (pCS == NULL)
        return TRUE;

    // Return FALSE if there are any overrides or asserts
    if (pCS->GetOverridesCount() > 0)
        return FALSE;
    // Return FALSE if there is an inner CS
    if (pCS->m_CSAD != ADID(INVALID_APPDOMAIN_ID))
        return FALSE;
     
     for(DWORD i=0; i < m_ADStack.GetNumDomains(); i++)
     {
         DomainCompressedStack *pDCS = (DomainCompressedStack *) m_DCSList[i];
         if (!IsDCSContained(pDCS))
            return FALSE;
     }
     return TRUE;
     
}


// If there is a compressed stack present in the captured CompressedStack, return that CS in the current domain
OBJECTREF NewCompressedStack::GetCompressedStackInner()
{
    _ASSERTE(m_CSAD != ADID(INVALID_APPDOMAIN_ID));

    AppDomain* pCurrentDomain = GetAppDomain();
    NewArrayHolder<BYTE> pbtmpSerializedObject(NULL);
    DWORD cbtmpSerializedObject = 0;


    OBJECTREF refRetVal = NULL;

    if (pCurrentDomain->GetId()== m_CSAD)
    {
        // we're in the right domain already 
        if (m_pCtxTxFrame == NULL)
        {
            // Get CS from the thread
            refRetVal = GetThread()->GetCompressedStack();
        }
        else
        {
            // Get CS from a Ctx transition frame
            refRetVal = (OBJECTREF)SecurityStackWalk::GetCSFromContextTransitionFrame(m_pCtxTxFrame);
            _ASSERTE(refRetVal != NULL); //otherwise we would not have saved the frame in the CB data
        }
    }
    else
    {
        GCPROTECT_BEGIN (refRetVal);          
        // need to marshal the CS over into the current AD
        ENTER_DOMAIN_ID(m_CSAD);
        {
            if (m_pCtxTxFrame == NULL)
            {

                // Get CS from the thread
                refRetVal = GetThread()->GetCompressedStack();
            }
            else
            {
                // Get CS from a Ctx transition frame
                refRetVal = (OBJECTREF)SecurityStackWalk::GetCSFromContextTransitionFrame(m_pCtxTxFrame);
                _ASSERTE(refRetVal != NULL); //otherwise we would not have saved the frame in the CB data
            }
            AppDomainHelper::MarshalObject(GetAppDomain(), &refRetVal, &pbtmpSerializedObject, &cbtmpSerializedObject);
        }
        END_DOMAIN_TRANSITION
        refRetVal = NULL;
        AppDomainHelper::UnmarshalObject(pCurrentDomain,pbtmpSerializedObject, cbtmpSerializedObject, &refRetVal);
        GCPROTECT_END ();
        _ASSERTE(refRetVal != NULL); //otherwise we would not have saved the frame in the CB data
    }

    return refRetVal;

}

// == Now begin the functions used in building Demand evaluation of a compressed stack
FCIMPL1(DWORD, NewCompressedStack::FCallGetDCSCount, SafeHandle* hcsUNSAFE)
{
    CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    DWORD dwRet = 0;
    if (hcsUNSAFE != NULL)
    {
        SAFEHANDLE hcsSAFE = (SAFEHANDLE) hcsUNSAFE;

        HELPER_METHOD_FRAME_BEGIN_RET_1(hcsSAFE);     

        NewCompressedStack* ncs = (NewCompressedStack *)hcsSAFE->GetHandle();
        
        dwRet = ncs->m_ADStack.GetNumDomains();
        HELPER_METHOD_FRAME_END();
    }

    return dwRet;
    
}
FCIMPLEND


FCIMPL2(FC_BOOL_RET, NewCompressedStack::FCallIsImmediateCompletionCandidate, SafeHandle* hcsUNSAFE, OBJECTREF *innerCS)
{
    CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    BOOL bRet = FALSE;
    if (hcsUNSAFE != NULL)
    {
        SAFEHANDLE hcsSAFE = (SAFEHANDLE) hcsUNSAFE;

        HELPER_METHOD_FRAME_BEGIN_RET_1(hcsSAFE); 

        *innerCS = NULL;

        NewCompressedStack* ncs = (NewCompressedStack *)hcsSAFE->GetHandle();

        if (ncs != NULL)
        {
            // Non-FT case

            // Is there an inner CS?
            BOOL bHasCS = (ncs->m_CSAD != ADID(INVALID_APPDOMAIN_ID));

            // Is there is a DCS not in the current AD
            BOOL bHasOtherAppDomain = FALSE;
            if (ncs->m_DCSList != NULL)
            {
                for(DWORD i=0; i < ncs->m_ADStack.GetNumDomains(); i++)
                {
                    DomainCompressedStack* dcs = ncs->m_DCSList[i];
                    if (dcs != NULL && dcs->GetMyDomain() != GetAppDomain()->GetId())
                    {
                        bHasOtherAppDomain = TRUE;
                        break;
                    }
                }
            }
            if (bHasCS)
            {


                *innerCS = ncs->GetCompressedStackInner();
                ncs->m_pCtxTxFrame = NULL; // Clear the CtxTxFrame ASAP
         
            }
            bRet = bHasOtherAppDomain||bHasCS;
        }
        
        HELPER_METHOD_FRAME_END();
    }
    
    FC_RETURN_BOOL(bRet);
}
FCIMPLEND


FCIMPL2(Object*, NewCompressedStack::GetDomainCompressedStack, SafeHandle* hcsUNSAFE, DWORD index)
{
    CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;        
    }
    CONTRACTL_END;




    OBJECTREF refRetVal = NULL;
    if (hcsUNSAFE != NULL)
    {
        SAFEHANDLE hcsSAFE = (SAFEHANDLE) hcsUNSAFE;

        HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, refRetVal, hcsSAFE); 



        NewCompressedStack* ncs = (NewCompressedStack *)hcsSAFE->GetHandle();

        // First we check to see if the DCS at index i has a blob. If so, deserialize it into the current AD and return it. Else try create it
        DomainCompressedStack* dcs = ncs->m_DCSList[index];
        if (dcs != NULL)
        {
            refRetVal = dcs->GetDomainCompressedStackInternal(NULL);
        }
        
        HELPER_METHOD_FRAME_END();
    }

    return OBJECTREFToObject(refRetVal);
   
}
FCIMPLEND

FCIMPL1(void, NewCompressedStack::DestroyDCSList, SafeHandle* hcsUNSAFE)
{
    CONTRACTL
    {
        THROWS;
        DISABLED(GC_TRIGGERS);
        MODE_COOPERATIVE;
        SO_TOLERANT;
    }
    CONTRACTL_END;

    SAFEHANDLE hcsSAFE = (SAFEHANDLE) hcsUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_1(hcsSAFE); 

    NewCompressedStack* ncs = (NewCompressedStack *)hcsSAFE->GetHandle();

    ncs->Destroy(TRUE);

    HELPER_METHOD_FRAME_END();

}
FCIMPLEND
    
