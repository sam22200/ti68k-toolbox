// Export the actual exploration state, independently of the Python disc reader.
#include <stdio.h>
#include "../fft.h"
#include "../../../runtime/platform-sw/rt_sw.h"
int main(void)
{
    unsigned i;
    sw_init(62);
    for (i = 0; i < NUNIT; i++)
        printf("%u %u %u %u\n", i, st.unit[i].x, st.unit[i].z, st.unit[i].face);
    return 0;
}
