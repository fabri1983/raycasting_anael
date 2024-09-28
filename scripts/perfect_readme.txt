perfect - Minimal Perfect Hashing tool
Athor: Bob Jenkins
https://burtleburtle.net/bob/hash/perfect.html
A newer version is here https://github.com/driedfruit/jenkins-minimal-perfect-hash.


How to compile the project in Windows:
https://code.visualstudio.com/docs/cpp/config-mingw


Commands I'm using:

    perfect -dmf values_deltaDist < values_deltaDist.txt
    perfect -dmf values_rayDirAngle < values_rayDirAngle.txt

Options: d = decimal keys, p = perfect hash, m = minimal perfect hash, b = decimal pairs, f = fast method
  d: decimal keys (in contrast with h (hex)).
  m: Minimal perfect hash. Hash values will be in 0..nkeys-1, and every possible hash value will have a matching key. 
     This is the default. The size of tab[] will be 3..8 bits per key.
  p: Perfect hash. Hash values will be in 0..n-1, where n the smallest power of 2 greater or equal to the number of keys. 
     The size of tab[] will be 1..4 bits per key.
  f: produces bigger tab[] but slighly faster phash function
  s: (slow, in contrast with f) produces smaller tab[] but slower phash function
