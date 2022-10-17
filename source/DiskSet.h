int Init_Diskset (int max);
int Quit_Diskset ();
// Return 0 if succ.
int DisksetOpen (char *name);
char *DisksetName ();
char *DisksetFolder ();
int DisksetNumItems ();
// Return 0 if succ.
int DisksetGetItem (int index, char **uname, char **fname);
// Return 0 if succ.
int DisksetGetItem (char *uname, char **fname);
void DisksetGetDefault (int *d1, int *d2);
