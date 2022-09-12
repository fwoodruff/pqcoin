#  Post-quantum Coin
This project is currently a work in progress.
The goal is to write a proof-of-work cryptocurrency resistant to attacks from quantum computers.

Quantum computers enable private key recovery for all popular crytocurrencies. This completely breaks cryptocurrencies.

FALCON (falcon-sign.info/) is based on the hardness of the short integer solution problem, not discrete logarithms.
FALCON simply doesn't do the thing quantum computer break.

I want public and private keys to be small and reusable. This is how I differentiate myself from other post-quantum cryptocurrencies.

FALCON in key recovery mode allows 32 byte (quantum preimage resistant) CSPRNG seeds as private keys, and 48 byte (quantum collision resistant) hashes as public keys. The catch is much larger signatures than we are used to (many kilobytes per signature), but we can safely delete the signatures once buried under lots of confirmed blocks.

Grover's algorithm mining enables 51% attacks, which can reverse transactions.
If physical transport and the internet shut down for weeks **and** the majority of mature quantum computers are used maliciously, coins may be stolen. This is implausible.
