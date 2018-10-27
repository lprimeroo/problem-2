main()
{
    /* Define default values: */

    int n = 0;
    float x = 0.0;

    /* Define contents of dialog window */

    create_int_dialog_entry("n", &n);
    create_float_dialog_entry("x", &x);

    /* Create window with name "Setup" and top-left corner at (0,0) */

    set_up_dialog("Setup", 0, 0);

    /* Display the window and read the results */

    read_dialog_window();

    /* Print out the new values */
    for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
    printf("n = %d, x = %f\n", n, x);
}

