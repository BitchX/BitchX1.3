/*
 * File: ./EuropaAI/_EuropaImplBase.java
 * From: europa.idl
 * Date: Thu Dec 23 02:08:42 1999
 *   By: idltojava Java IDL 1.2 Aug 11 1998 02:00:18
 */

package EuropaAI;
public abstract class _EuropaImplBase extends org.omg.CORBA.DynamicImplementation implements EuropaAI.Europa {
    // Constructor
    public _EuropaImplBase() {
         super();
    }
    // Type strings for this class and its superclases
    private static final String _type_ids[] = {
        "IDL:EuropaAI/Europa:1.0"
    };

    public String[] _ids() { return (String[]) _type_ids.clone(); }

    private static java.util.Dictionary _methods = new java.util.Hashtable();
    static {
      _methods.put("inputChat", new java.lang.Integer(0));
     }
    // DSI Dispatch call
    public void invoke(org.omg.CORBA.ServerRequest r) {
       switch (((java.lang.Integer) _methods.get(r.op_name())).intValue()) {
           case 0: // EuropaAI.Europa.inputChat
              {
              org.omg.CORBA.NVList _list = _orb().create_list(0);
              org.omg.CORBA.Any _text = _orb().create_any();
              _text.type(org.omg.CORBA.ORB.init().get_primitive_tc(org.omg.CORBA.TCKind.tk_string));
              _list.add_value("text", _text, org.omg.CORBA.ARG_IN.value);
              r.params(_list);
              String text;
              text = _text.extract_string();
                            this.inputChat(text);
              org.omg.CORBA.Any __return = _orb().create_any();
              __return.type(_orb().get_primitive_tc(org.omg.CORBA.TCKind.tk_void));
              r.result(__return);
              }
              break;
            default:
              throw new org.omg.CORBA.BAD_OPERATION(0, org.omg.CORBA.CompletionStatus.COMPLETED_MAYBE);
       }
 }
}
