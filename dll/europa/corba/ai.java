import java.io.*;
import org.omg.CORBA.*;
import EuropaAI.*;

class EuropaServant extends _EuropaImplBase {
    public void inputChat(String text) {
	System.out.println(text);
    }
}

public class ai {
    public static void main(String args[]) {
	try {
	    ORB orb = ORB.init(args, null);

	    // create the servant and register it
	    EuropaServant europaRef = new EuropaServant();
	    orb.connect(europaRef);

	    // stringify the europaRef and dump to an ior file
	    String str = orb.object_to_string(europaRef);
	    FileOutputStream fos = new FileOutputStream("ai.ior");
	    PrintStream ps = new PrintStream(fos);
	    ps.print(str);
	    ps.close();

            // wait for invocations from clients
            java.lang.Object sync = new java.lang.Object();
            synchronized (sync) {
                sync.wait();
            }
	    
        } catch (Exception e) {
            System.err.println("Error: " + e);
            e.printStackTrace(System.out);
        }
    }
}
