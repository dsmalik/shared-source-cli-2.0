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
using System;
using System.IO;
using System.Collections;
public class Co3936ToArray_tp
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "ArrayList.ToArray()";
    public static String s_strTFName        = "Co3936ToArray_tp.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        ArrayList alst1;
        String strValue;
        Array arr1;
        try 
        {
            do
            {
                strLoc = "Loc_8345vdfv";
                alst1 = new ArrayList();
                for(int i=0; i<10; i++)
                {
                    strValue = "String_" + i;
                    alst1.Add(strValue);
                }
                arr1 = alst1.ToArray(typeof(String));
                iCountTestcases++;
                for(int i=0; i<10; i++)
                {
                    strValue = "String_" + i;
                    if(!strValue.Equals((String)arr1.GetValue(i))) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_561dvs_" + i + "! Expected value not returned, " + strValue);
                    }
                }
                alst1 = new ArrayList();
                arr1 = alst1.ToArray(typeof(Object));
                iCountTestcases++;
                if(arr1.Length!=0) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_5434gbfg! Expected value not returned, " + arr1.Length);
                }
                strLoc = "Loc_8345vdfv";
                alst1 = new ArrayList();
                for(int i=0; i<10; i++)
                {
                    strValue = "String_" + i;
                    alst1.Add(strValue);
                }
                try 
                {
                    iCountTestcases++;
                    arr1 = alst1.ToArray(typeof(Int32));
                    iCountErrors++;
                    Console.WriteLine("Err_8452vs! exception not thrown");
                }
                catch(InvalidCastException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_13753vdf! Unexpected exception, " + ex.ToString());
                }
                try 
                {
                    iCountTestcases++;
                    arr1 = alst1.ToArray(null);
                    iCountErrors++;
                    Console.WriteLine("Err_8452vs! exception not thrown");
                }
                catch(ArgumentNullException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_13753vdf! Unexpected exception, " + ex.ToString());
                }
            } while (false);
        } 
        catch (Exception exc_general ) 
        {
            ++iCountErrors;
            Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general==\n"+exc_general.ToString());
        }
        if ( iCountErrors == 0 )
        {
            Console.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
            return true;
        }
        else
        {
            Console.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3936ToArray_tp cbA = new Co3936ToArray_tp();
        try 
        {
            bResult = cbA.runTest();
        } 
        catch (Exception exc_main)
        {
            bResult = false;
            Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main);
        }
        if (!bResult)
        {
            Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
            Console.WriteLine( " " );
            Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode=0; else Environment.ExitCode=1;
    }
}
internal class Foo 
{
    internal String strValue;
    internal Foo()
    {
        strValue = "Hello World";
    }
}
