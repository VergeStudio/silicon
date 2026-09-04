#ifdef SFFI_EXEC_STATIC_TRAMP

#    define PPC_TRAMP_MAP_SHIFT 16
#    define PPC_TRAMP_MAP_SIZE (1 << PPC_TRAMP_MAP_SHIFT)
#    ifdef __PCREL__
#        define PPC_TRAMP_SIZE 24
#    else
#        define PPC_TRAMP_SIZE 40
#    endif 
#endif     
