
void EdsTableTool_GENERATOR(void)
{
m4_include(TBLDEF_C)

    EdsTableTool_DoExport(&CFE_TBL_FileDef, &OBJNAME, sizeof(OBJNAME), "TYPENAME", sizeof(TYPENAME));
}
