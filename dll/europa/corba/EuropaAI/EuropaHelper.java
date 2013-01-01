/*
 * File: ./EuropaAI/EuropaHelper.java
 * From: europa.idl
 * Date: Thu Dec 23 02:08:42 1999
 *   By: idltojava Java IDL 1.2 Aug 11 1998 02:00:18
 */

package EuropaAI;
public class EuropaHelper {
     // It is useless to have instances of this class
     private EuropaHelper() { }

    public static void write(org.omg.CORBA.portable.OutputStream out, EuropaAI.Europa that) {
        out.write_Object(that);
    }
    public static EuropaAI.Europa read(org.omg.CORBA.portable.InputStream in) {
        return EuropaAI.EuropaHelper.narrow(in.read_Object());
    }
   public static EuropaAI.Europa extract(org.omg.CORBA.Any a) {
     org.omg.CORBA.portable.InputStream in = a.create_input_stream();
     return read(in);
   }
   public static void insert(org.omg.CORBA.Any a, EuropaAI.Europa that) {
     org.omg.CORBA.portable.OutputStream out = a.create_output_stream();
     write(out, that);
     a.read_value(out.create_input_stream(), type());
   }
   private static org.omg.CORBA.TypeCode _tc;
   synchronized public static org.omg.CORBA.TypeCode type() {
          if (_tc == null)
             _tc = org.omg.CORBA.ORB.init().create_interface_tc(id(), "Europa");
      return _tc;
   }
   public static String id() {
       return "IDL:EuropaAI/Europa:1.0";
   }
   public static EuropaAI.Europa narrow(org.omg.CORBA.Object that)
	    throws org.omg.CORBA.BAD_PARAM {
        if (that == null)
            return null;
        if (that instanceof EuropaAI.Europa)
            return (EuropaAI.Europa) that;
	if (!that._is_a(id())) {
	    throw new org.omg.CORBA.BAD_PARAM();
	}
        org.omg.CORBA.portable.Delegate dup = ((org.omg.CORBA.portable.ObjectImpl)that)._get_delegate();
        EuropaAI.Europa result = new EuropaAI._EuropaStub(dup);
        return result;
   }
}
