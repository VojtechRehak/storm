#include "../StormEigen/Core"

#ifdef STORMEIGEN_SHOULD_FAIL_TO_BUILD
#define CV_QUALIFIER const
#else
#define CV_QUALIFIER
#endif

using namespace StormEigen;

void foo(CV_QUALIFIER Matrix3d &m){
    Block<Matrix3d> b(m,0,0,3,3);
}

int main() {}
