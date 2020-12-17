#include <Application.h>
#include <TransferFunction.h>

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

class TestMsg : public Work
{
public:
    TestMsg(TransferFunctionP tfp, int n, float *v) : TestMsg(sizeof(Key) + sizeof(int) + n*sizeof(float*))
    {
        unsigned char *p = (unsigned char *)get();

        *(Key *)p = tfp->getkey();
        p = p + sizeof(Key);

        *(int *)p = n;
        p = p + sizeof(int);

        memcpy((void *)p, v, n*sizeof(float));
        p = p + n*sizeof(float);
    }

    ~TestMsg() {}
    WORK_CLASS(TestMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        unsigned char *p = (unsigned char *)get();

        TransferFunctionP tfp = TransferFunction::GetByKey(*(Key *)p);
        p = p + sizeof(Key);

        int n = *(int *)p;
        p = p + sizeof(int);

        float *v = (float *)p;

        int first = mpiRank * (n / mpiSize);
        int last  = (mpiRank == mpiSize-1) ? n - 1 : ((mpiRank + 1) * (n / mpiSize)) - 1;

        int k = (last - first) + 1;
        float *r = new float[tfp->GetWidth() * k];

        ispc::TransferFunction_Test(tfp->GetISPC(), k, v+first, r);

        for (int i = 0; i < mpiSize; i++)
        {
            if (i == mpiRank)
            {
                for (int j = 0; j < k; j++)
                {
                    std::cerr << (j+first) << " (" << v[j+first] << "): ";
                    for (int l = 0; l < tfp->GetWidth(); l++)
                        std::cerr << r[j*tfp->GetWidth() + l] << " ";
                    std::cerr << "\n";
                }
            }
            MPI_Barrier(c);
        }

        delete[] r;
        
        return false;
    }
};

WORK_CLASS_TYPE(TestMsg)

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    std::cerr << "syntax: " << a << " [options] " << endl;
    std::cerr << "options:" << endl;
    std::cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  }
  exit(1);
}

int
main(int argc, char *argv[])
{
    char *dbgarg;
    bool dbg = false;
    bool atch = false;

    Application theApplication(&argc, &argv);
    theApplication.Start();

    for (int i = 1; i < argc; i++)
        if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
        else syntax(argv[0]);

    mpiRank = theApplication.GetRank();
    mpiSize = theApplication.GetSize();

    Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

    theApplication.Run();

    TransferFunction::Register();
    TestMsg::Register();

    if (mpiRank == 0)
    {
        // data from min - 1 to max + 1: -1 to mpiSize+1

        float *values = new float[100];
        for (int i = 0; i < 100; i++)
            values[i] = -1.0 + (mpiSize + 2)*(i / 99.0);

        TransferFunctionP tf = TransferFunction::NewP();

        int widths[] = {1, 3, 4};
        for (int i = 0; i < 3; i++)
        {
            int w = widths[i];

            float data[w*256];

            float *p = data;
            for (int i = 0; i < 256; i++)
            {
                float d = i / 255.0;
                for (int j = 1; j <= w; j++)
                    *p++ = d*j;
            }

            tf->Set(256, w, 0.0, (float)mpiSize, data);
            tf->Commit();

            TestMsg t(tf, 100, values);
            t.Broadcast(true, true);
        }

        theApplication.QuitApplication();
    }

    theApplication.Wait();
}




