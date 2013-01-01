/*
 * File: ./EuropaAI/_EuropaStub.java
 * From: europa.idl
 * Date: Thu Dec 23 02:08:42 1999
 *   By: idltojava Java IDL 1.2 Aug 11 1998 02:00:18
 */

package EuropaAI;
public class _EuropaStub
	extends org.omg.CORBA.portable.ObjectImpl
    	implements EuropaAI.Europa {

    public _EuropaStub(org.omg.CORBA.portable.Delegate d) {
          super();
          _set_delegate(d);
    }

    private static final String _type_ids[] = {
        "IDL:EuropaAI/Europa:1.0"
    };

    public String[] _ids() { return (String[]) _type_ids.clone(); }

    //	IDL operations
    //	    Implementation of ::EuropaAI::Europa::inputChat
    public void inputChat(String text)
 {
           org.omg.CORBA.Request r = _request("inputChat");
           org.omg.CORBA.Any _text = r.add_in_arg();
           _text.insert_string(text);
           r.invoke();
   }

};
