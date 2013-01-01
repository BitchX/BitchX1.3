/*
 * File: ./EuropaAI/EuropaHolder.java
 * From: europa.idl
 * Date: Thu Dec 23 02:08:42 1999
 *   By: idltojava Java IDL 1.2 Aug 11 1998 02:00:18
 */

package EuropaAI;
public final class EuropaHolder
     implements org.omg.CORBA.portable.Streamable{
    //	instance variable 
    public EuropaAI.Europa value;
    //	constructors 
    public EuropaHolder() {
	this(null);
    }
    public EuropaHolder(EuropaAI.Europa __arg) {
	value = __arg;
    }

    public void _write(org.omg.CORBA.portable.OutputStream out) {
        EuropaAI.EuropaHelper.write(out, value);
    }

    public void _read(org.omg.CORBA.portable.InputStream in) {
        value = EuropaAI.EuropaHelper.read(in);
    }

    public org.omg.CORBA.TypeCode _type() {
        return EuropaAI.EuropaHelper.type();
    }
}
