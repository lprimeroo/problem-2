    int main()
    {
        int i, sum = 0;
       
        for ( i = 1; i <= LAST; i++ ) {
          sum += i;
        } /*-for-*/
        printf("sum = %d\n", sum);
        for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
        return 0;
    }

