#include <stdint.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #undef GetObject
    #include <intrin.h>

    extern "C" void _ReadWriteBarrier();
    #pragma intrinsic(_ReadWriteBarrier)
    #pragma intrinsic(_InterlockedCompareExchange)
    #pragma intrinsic(_InterlockedExchangeAdd)

    // Memory Barriers to prevent CPU and Compiler re-ordering
    #define BASE_MEMORYBARRIER_ACQUIRE() _ReadWriteBarrier()
    #define BASE_MEMORYBARRIER_RELEASE() _ReadWriteBarrier()
    #define BASE_ALIGN(x) __declspec( align( x ) ) 

#else
    #define BASE_MEMORYBARRIER_ACQUIRE() __asm__ __volatile__("": : :"memory")  
    #define BASE_MEMORYBARRIER_RELEASE() __asm__ __volatile__("": : :"memory")  
    #define BASE_ALIGN(x)  __attribute__ ((aligned( x )))
#endif

// Atomically performs: if( *pDest == compareWith ) { *pDest = swapTo; }
namespace Urho3D
{
    // returns old *pDest (so if successfull, returns compareWith)
    inline unsigned AtomicCompareAndSwap( volatile unsigned* pDest, unsigned swapTo, unsigned compareWith )
    {
       #ifdef _WIN32
            // assumes two's complement - unsigned / signed conversion leads to same bit pattern
            return _InterlockedCompareExchange( (volatile long*)pDest,swapTo, compareWith );
        #else
            return __sync_val_compare_and_swap( pDest, compareWith, swapTo );
        #endif      
    }

    inline long long unsigned AtomicCompareAndSwap( volatile long long unsigned* pDest, long long unsigned swapTo, long long unsigned compareWith )
    {
       #ifdef _WIN32
            // assumes two's complement - unsigned / signed conversion leads to same bit pattern
            return _InterlockedCompareExchange64( (__int64 volatile*)pDest, swapTo, compareWith );
        #else
            return __sync_val_compare_and_swap( pDest, compareWith, swapTo );
        #endif      
    }   

    // Atomically performs: tmp = *pDest; *pDest += value; return tmp;
    inline int AtomicAdd( volatile int* pDest, int value )
    {
       #ifdef _WIN32
            return _InterlockedExchangeAdd( (long*)pDest, value );
        #else
            return __sync_add_and_fetch( pDest, value );
        #endif      
    }
}
