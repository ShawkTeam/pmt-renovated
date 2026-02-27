SECTIONS {
    .builtin_modules : {
        __start_builtin_modules = .;
        *(.builtin_modules)
        __end_builtin_modules = .;
    }
}

INSERT AFTER .text;
