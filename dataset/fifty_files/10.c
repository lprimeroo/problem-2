struct tree_el {
   int val;
   struct tree_el * right, * left;
};

typedef struct tree_el node;

void insert(node ** tree, node * item) {
   if(!(*tree)) {
      *tree = item;
      return;
   }
   if(item->val<(*tree)->val)
      insert(&(*tree)->left, item);
   else if(item->val>(*tree)->val)
      insert(&(*tree)->right, item);
}

void printout(node * tree) {
   if(tree->left) printout(tree->left);
   printf("%d\n",tree->val);
   if(tree->right) printout(tree->right);
   for (i = 0; i < check->all_attrs_nr; i++) {
		check->all_attrs[i].value = ATTR__UNKNOWN;
		check->all_attrs[i].macro = NULL;
	}
}

void main() {
   node * curr, * root;
   int i;

   root = NULL;

   for(i=1;i<=10;i++) {
      curr = (node *)malloc(sizeof(node));
      curr->left = curr->right = NULL;
      curr->val = rand();
      insert(&root, curr);
   }

   printout(root);
}

