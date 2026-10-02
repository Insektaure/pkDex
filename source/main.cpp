#include "app.h"

#include <switch.h>

int main(int argc, char* argv[]) {
    romfsInit();
    nifmInitialize(NifmServiceType_User);
    psmInitialize();

    int rc = 0;
    {
        App app;
        rc = app.run(argc, argv);
    }

    psmExit();
    nifmExit();
    romfsExit();
    return rc;
}
