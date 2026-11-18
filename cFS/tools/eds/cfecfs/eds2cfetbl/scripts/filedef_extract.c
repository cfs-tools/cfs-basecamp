#include <stdio.h>
#include <cfe_tbl_filedef.h>

extern CFE_TBL_FileDef_t CFE_TBL_FileDef;

int main(void)
{
    printf("%s\n", CFE_TBL_FileDef.ObjectName);
    return 0;
}