So about 1 year ago I started working on this online multiplayer party game using Jolt physics to drive pretty much all gameplay https://store.steampowered.com/app/4311700/Ignited/

The game is built using Unreal 5. The characters in the game are fully simulated physics objects. They carry hammers that you control using your mouse. By swinging the hammer you get to move your character around, there is no other way to move. Kind of like the infamous game "Getting Over It" by Bennett Foddy. But on top of that you are also fighting the other player for objectives or just in a classic deathmatch.

The more interesting part is that online multiplayer works by exchanging player inputs ONLY. Game state is never exchanged between clients. That is achieved by ensuring the game is fully deterministic and produces the same exact output on multiple machines given the same set of inputs.

The game is also 2D. It really put Jolt to the test on that front and on determinism. I can say that in both of those fronts it has never failed me. I've fixed hundreds of bugs and in every single case it was my own code that was at fault.

So if anyone is thinking of using Jolt for a deterministic online multiplayer game - it's absolutely solid for that. Once you understand how it all works and you have your own games framework working with rollbacks, it is a joy to add new features to multiplayer. Normally you'd have to think about replication and client side prediction with every little new thing you want to add to your game. But when you're only exchanging inputs over the wire you're really adding features like you would to a single player game. At least to a certain extent.

Having said that, setting it all up and fixing all the bugs was a huge technical challenge. I was about 8-9 months into development when I finally caught the last few nasty bugs and online multiplayer started working in a stable manner. So it's a huge upfront technical cost.

The game was meant to be a commercial release and I was planning to share it here once it was out, however I'm not sure about its future anymore. I ran into some serious issues during playtesting. Nothing to do with Jolt or online play. The issue is most players find the controls unintuitive and way too difficult. I improved them a lot based on feedback, but it's just not enough and I've lost faith. New players just bounce off the game as soon as they try to move. Feel free to request a playtest on Steam if you want to give the game a shot yourself. It should grant access instantly.

I want to thank jrouwe for making this amazing library and giving it to us for free. It is an amazing piece of engineering. I've had to go through a lot of the source code to solve issues and the code is super clean and readable. Thank you!
