import net.sourceforge.mltpp.*;

public class Play {

	static {
		System.loadLibrary("mltpp_java");
	}

	public static void main (String[] args) {

		// Start the mlt system
		Factory.init( null );

		// Create the producer
		Producer p = new Producer( args[0], null );

		if ( p.is_valid() ) {
	  
			p.set ("eof", "loop");
	  
			// Create the consumer
			Consumer c = new Consumer("sdl", null);

			// Turn off the default rescaling
			c.set("rescale", "none");
	
			// Connect the producer to the consumer
			c.connect(p);
	
			// Start the consumer
			c.start();

			// Wait until the user stops the consumer
			Object o = new Object();
			while (c.is_stopped() == 0) {
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
