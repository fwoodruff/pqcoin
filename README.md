#  Post-quantum Coin
This project is currently a work in progress.
The goal is to write a proof-of-work cryptocurrency resistant to attacks from quantum computers.

The two attacks quantum computers enable are 51% attacks and private key recovery.
Deep chain reorganisations may reverse blocks but may not break integrity of chain.
If there is only one computer capable of Grover's algorithm mining, and the device is highly mature and efficient, and the attacker simultaneously performs a massive DDoS attack on all nodes for weeks at a time, then unsigned transactions may be accepted by the network. It would be simpler to obtain coins through military force.

FALCON (falcon-sign.info/) is based on the hardness of the short integer solution problem, not discrete logarithms.
FALCON simply doesn't do the thing quantum computer break.

I want public and private keys to be small and reusable. This is how I differentiate myself from other post-quantum cryptocurrencies.

FALCON in key recovery mode allows 32 byte (quantum preimage resistant) CSPRNG seeds as private keys, and 48 byte (quantum collision resistant) hashes as public keys. The catch is much larger signatures than we are used to (many kilobytes per signature), but we can safely delete the signatures once buried under lots of confirmed blocks.
