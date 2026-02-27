SECTIONS {
    .pmt_module : {
        __start_pmt_module = .;
        *(.pmt_module)
        __end_pmt_module = .;
    }
}

INSERT AFTER .text;
