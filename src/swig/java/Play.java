import org.mltframework.*;

public class Play {

	static {
		System.loadLibrary("mlt_java");
	}

	public static void main (String[] args) {

		// Start the mlt system
		Factory.init( null );

		// Set the output profile
		Profile profile = new Profile( "" );

		// Create the producer
		Producer p = new Producer( profile, args[0], null );

		if ( p.is_valid() ) {
	  
			p.set ("eof", "loop");
	  
			// Create the consumer
			Consumer c = new Consumer( profile, "sdl", null);

			// Turn off the default rescaling
			c.set("rescale", "none");
	
			// Connect the producer to the consumer
			c.connect(p);
	
			// Start the consumer
			c.start();

			// Wait until the user stops the consumer
			Object o = new Object();
			while ( !c.is_stopped() ) {
				synchronized (o) {
					try {
						o.wait(1000);
					} catch (InterruptedException e) {
						// ignored
					}
				}
			}

			// Stop it anyway
			c.stop();
		} else {
			System.out.println ("Unable to open " + args[0]);
		}
	}
}
