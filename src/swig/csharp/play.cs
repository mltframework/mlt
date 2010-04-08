using System;
using System.Threading;
using Mlt;

public class Play {
	public static void Main(String[] args) {
		Console.WriteLine("Welcome to MLT.");
		Factory.init();
		Profile profile = new Profile("");
		Producer p = new Producer(profile, args[0], null);
		if (p.is_valid()) {
			Consumer c = new Consumer(profile, "sdl", null);
			c.set("rescale", "none");
			c.connect(p);
			c.start();
			while (!c.is_stopped())
				Thread.Sleep(300);
			c.stop();
		}
	}
}
