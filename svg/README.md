This file contains the core of the SVGAndMe code.

In order to use it in your application, just include "svg.h" in your main program.

Note:  There is some initialization that needs to occur to use the code.  You must
create a single instance of the SVGFactory object in your code before making any
usage of the svg parser.

```C++
#include "svg.h"

SVGFactory gFactory;

void main() {...}
```
