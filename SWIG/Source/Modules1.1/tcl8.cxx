/* -----------------------------------------------------------------------------
 * tcl8.cxx
 *
 *     Tcl8.0 wrapper module.
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "swig11.h"
#include "tcl8.h"
#include <ctype.h>

static char *usage = (char*)"\
Tcl 8.0 Options (available with -tcl)\n\
     -module name    - Set name of module\n\
     -prefix name    - Set a prefix to be appended to all names\n\
     -namespace      - Build module into a Tcl 8 namespace. \n\
     -noobject       - Omit code for object oriented interface.\n\n";

static String     *mod_init = 0;
static String     *cmd_info = 0;
static String     *var_info = 0;
static String     *methods = 0;
static String     *attributes = 0;

static String     *prefix = 0;
static String     *module = 0;
static int        nspace = 0;
static int        shadow = 1;
static String    *init_name = 0;
static String    *ns_name = 0;
static int        have_constructor;
static int        have_destructor;

static String    *class_name = 0;
static String    *class_type = 0;
static String    *real_classname = 0;
static Hash      *repeatcmd = 0;


/* -----------------------------------------------------------------------------
 * TCL8::parse_args()
 * ----------------------------------------------------------------------------- */

void
TCL8::parse_args(int argc, char *argv[]) {
  int i;
  Swig_swiglib_set("tcl");

  for (i = 1; i < argc; i++) {
      if (argv[i]) {
	  if (strcmp(argv[i],"-prefix") == 0) {
	    if (argv[i+1]) {
	      prefix = NewString(argv[i+1]);
	      Swig_mark_arg(i);
	      Swig_mark_arg(i+1);
	      i++;
	    } else Swig_arg_error();
	  } else if (strcmp(argv[i],"-namespace") == 0) {
	    nspace = 1;
	    Swig_mark_arg(i);
          } else if (strcmp(argv[i],"-noobject") == 0) {
	    shadow = 0;
	    Swig_mark_arg(i);
	  } else if (strcmp(argv[i],"-help") == 0) {
	    fputs(usage,stderr);
	  }
      }
  }

  if ((nspace) && module) {
    ns_name = Copy(module);
  } else if (prefix) {
    ns_name = Copy(prefix);

  }
  if (prefix)
    Append(prefix,"_");

  Preprocessor_define((void *) "SWIGTCL 1",0);
  Preprocessor_define((void *) "SWIGTCL8 1", 0);
  Swig_set_config_file("tcl8.swg");
}

/* -----------------------------------------------------------------------------
 * TCL8::initialize()
 * ----------------------------------------------------------------------------- */

void
TCL8::initialize(String *modname) {

  mod_init   = NewString("");
  cmd_info   = NewString("");
  var_info   = NewString("");
  methods    = NewString("");
  attributes = NewString("");
  repeatcmd  = NewHash();

  Swig_banner(f_runtime);

  /* Include a Tcl configuration file */
  if (NoInclude) {
    Printf(f_runtime,"#define SWIG_NOINCLUDE\n");
  }
  if (Swig_insert_file("common.swg",f_runtime) == -1) {
    Printf(stderr,"SWIG : Fatal error. Unable to locate 'common.swg' in SWIG library.\n");
    Swig_exit (EXIT_FAILURE);
  }
  if (Swig_insert_file("swigtcl8.swg",f_runtime) == -1) {
    Printf(stderr,"SWIG : Fatal error. Unable to locate 'swigtcl8.swg' in SWIG library.\n");
    Swig_exit (EXIT_FAILURE);
  }
  if (!module) module = NewString(modname);

  /* Fix capitalization for Tcl */
  char *c = Char(module);
  while (c && *c) {
    *c = (char) tolower(*c);
    c++;
  }
  /* Now create an initialization function */
  init_name = NewStringf("%s_Init",module);
  c = Char(init_name);
  *c = toupper(*c);

  if (!ns_name) ns_name = Copy(module);

  /* If namespaces have been specified, set the prefix to the module name */
  if ((nspace) && (!prefix)) {
    prefix = NewStringf("%s_",module);
  } else {
    prefix = NewString("");
  }

  Printf(f_header,"#define SWIG_init    %s\n", init_name);
  if (!module) module = NewString("swig");
  Printf(f_header,"#define SWIG_name    \"%s\"\n", module);
  if (nspace) {
    Printf(f_header,"#define SWIG_prefix  \"%s::\"\n", ns_name);
    Printf(f_header,"#define SWIG_namespace \"%s\"\n\n", ns_name);
  } else {
    Printf(f_header,"#define SWIG_prefix  \"%s\"\n", prefix);
    Printf(f_header,"#define SWIG_namespace \"\"\n\n");
  }
  Printf(f_header,"#ifdef __cplusplus\n");
  Printf(f_header,"extern \"C\" {\n");
  Printf(f_header,"#endif\n");
  Printf(f_header,"#ifdef MAC_TCL\n");
  Printf(f_header,"#pragma export on\n");
  Printf(f_header,"#endif\n");
  Printf(f_header,"SWIGEXPORT(int) %s(Tcl_Interp *);\n", init_name);
  Printf(f_header,"#ifdef MAC_TCL\n");
  Printf(f_header,"#pragma export off\n");
  Printf(f_header,"#endif\n");
  Printf(f_header,"#ifdef __cplusplus\n");
  Printf(f_header,"}\n");
  Printf(f_header,"#endif\n");

  Printf(f_init,"SWIGEXPORT(int) %s(Tcl_Interp *interp) {\n", init_name);
  Printf(f_init,"int i;\n");
  Printf(f_init,"if (interp == 0) return TCL_ERROR;\n");

  /* Check to see if we're adding support for Tcl8 nspaces */
  if (nspace) {
    Printf(f_init,"Tcl_Eval(interp,\"namespace eval %s { }\");\n", ns_name);
  }

  Printf(cmd_info, "\nstatic swig_command_info swig_commands[] = {\n");
  Printf(var_info, "\nstatic swig_var_info swig_variables[] = {\n");
  Printv(f_init,
	 "for (i = 0; swig_types_initial[i]; i++) {\n",
	 "swig_types[i] = SWIG_TypeRegister(swig_types_initial[i]);\n",
	 "}\n", 0);
}

/* -----------------------------------------------------------------------------
 * TCL8::close()
 * ----------------------------------------------------------------------------- */

void
TCL8::close(void) {

  Printv(cmd_info, tab4, "{0, 0, 0}\n", "};\n",0);
  Printv(var_info, tab4, "{0,0,0,0}\n", "};\n",0);

  Printf(f_wrappers,"%s", cmd_info);
  Printf(f_wrappers,"%s", var_info);

  Printf(f_init,"for (i = 0; swig_commands[i].name; i++) {\n");
  Printf(f_init,"Tcl_CreateObjCommand(interp, (char *) swig_commands[i].name, swig_commands[i].wrapper, swig_commands[i].clientdata, NULL);\n");
  Printf(f_init,"}\n");

  Printf(f_init,"for (i = 0; swig_variables[i].name; i++) {\n");
  Printf(f_init,"Tcl_SetVar(interp, (char *) swig_variables[i].name, \"\", TCL_GLOBAL_ONLY);\n");
  Printf(f_init,"Tcl_TraceVar(interp, (char *) swig_variables[i].name, TCL_TRACE_READS | TCL_GLOBAL_ONLY, swig_variables[i].get, (ClientData) swig_variables[i].addr);\n");
  Printf(f_init,"Tcl_TraceVar(interp, (char *) swig_variables[i].name, TCL_TRACE_WRITES | TCL_GLOBAL_ONLY, swig_variables[i].set, (ClientData) swig_variables[i].addr);\n");
  Printf(f_init,"}\n");

  /* Dump the pointer equivalency table */
  SwigType_emit_type_table(f_runtime, f_wrappers);

  /* Close the init function and quit */
  Printf(f_init,"return TCL_OK;\n");
  Printf(f_init,"}\n");
}

/* -----------------------------------------------------------------------------
 * TCL8::create_command()
 * ----------------------------------------------------------------------------- */

void
TCL8::create_command(String *cname, String *iname) {

  String *wname = Swig_name_wrapper(cname);
  Printv(cmd_info, tab4, "{ SWIG_prefix \"", iname, "\", ", wname, ", NULL},\n", 0);

  /* Add interpreter name to repeatcmd hash table.  This hash is used in C++ code
     generation to try and find repeated wrapper functions. */

  Setattr(repeatcmd,iname,wname);
}

/* -----------------------------------------------------------------------------
 * TCL8::create_function()
 * ----------------------------------------------------------------------------- */

void
TCL8::function(DOH *node) {
  char *name, *iname;
  SwigType *d;
  ParmList *l;
  Parm            *p;
  int              pcount,i,j;
  char            *tm;
  Wrapper         *f;
  String          *incode, *cleanup, *outarg, *argstr, *args;
  int              numopt= 0;

  name = GetChar(node,"name");
  iname = GetChar(node,"scriptname");
  d = Getattr(node,"type");
  l = Getattr(node,"parms");

  incode  = NewString("");
  cleanup = NewString("");
  outarg  = NewString("");
  argstr  = NewString("\"");
  args    = NewString("");

  f = NewWrapper();
  Printv(f,
	 "static int\n ", Swig_name_wrapper(iname), "(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {\n",
	 0);

  /* Print out variables for storing arguments. */
  pcount = emit_args(node, f);
  numopt = check_numopt(l);

  /* Extract parameters. */
  i = 0;
  j = 0;
  p = l;
  while (p != 0) {
    char      source[64];
    char      target[64];
    char      argnum[64];
    SwigType *pt = Gettype(p);
    String   *pn = Getname(p);

    /* Produce string representations of the source and target arguments */
    sprintf(source,"objv[%d]",j+1);
    sprintf(target,"%s", Char(Getlname(p)));
    sprintf(argnum,"%d",j+1);

    /* See if this argument is being ignored */
    if (!Getignore(p)) {
      if (j == (pcount-numopt)) Putc('|',argstr);
      if ((tm = Swig_typemap_lookup((char*)"in",pt,pn,source,target,f))) {
	Putc('o',argstr);
	Printf(args,",0");
	Printf(incode,"%s\n", tm);
	Replace(incode,"$argnum",argnum, DOH_REPLACE_ANY);
	Replace(incode,"$arg",source, DOH_REPLACE_ANY);
      } else {
	switch(SwigType_type(pt)) {
	case T_INT:
	case T_UINT:
	  Putc('i', argstr);
	  Printf(args,",&%s",target);
	  break;

	case T_BOOL:
	  Putc('i',argstr);
	  {
	    char tb[32];
	    sprintf(tb,"tempb%d",i);
	    Wrapper_add_localv(f,tb,"int",tb,0);
	    Printf(args,",&%s",tb);
	    Printv(incode, target, " = (bool) ", tb, ";\n", 0);
	  }
	  break;

	case T_SHORT:
	case T_USHORT:
	  Putc('h',argstr);
	  Printf(args,",&%s",target);
	  break;

	case T_LONG:
	case T_ULONG:
	  Putc('l',argstr);
	  Printf(args,",&%s",target);
	  break;

	case T_SCHAR:
	case T_UCHAR:
	  Putc('b',argstr);
	  Printf(args,",&%s", target);
	  break;

	case T_FLOAT:
	  Putc('f',argstr);
	  Printf(args,",&%s", target);
	  break;

	case T_DOUBLE:
	  Putc('d',argstr);
	  Printf(args,",&%s", target);
	  break;

	case T_CHAR :
	  Putc('c',argstr);
	  Printf(args,",&%s",target);
	  break;

	case T_VOID :
	  break;

	case T_USER:
	  SwigType_add_pointer(pt);
	  SwigType_remember(pt);
	  Putc('p',argstr);
	  Printv(args, ",&", target, ", SWIGTYPE", SwigType_manglestr(pt), 0);
	  SwigType_del_pointer(pt);
	  break;

	case T_STRING:
	  Putc('s',argstr);
	  Printf(args,",&%s",target);
	  break;

	case T_POINTER: case T_ARRAY: case T_REFERENCE:
	  {
	    SwigType *lt;
	    SwigType_remember(pt);
	    Putc('p',argstr);
	    lt = Swig_clocal_type(pt);
	    if (Cmp(lt,"p.void") == 0) {
	      Printv(args, ",&", target, ", 0", 0);
	    } else {
	      Printv(args, ",&", target, ", SWIGTYPE", SwigType_manglestr(pt), 0);
	    }
	    Delete(lt);
	    break;
	  }
	default :
	  Printf(stderr,"%s:%d: Unable to use type %s as a function argument.\n",
		 Getfile(node), Getline(node), SwigType_str(pt,0));
	  break;
	}
      }
      j++;
    }
    /* Check to see if there was any sort of a constaint typemap */
    if ((tm = Swig_typemap_lookup((char*)"check",pt,pn,source,target,0))) {
      Printf(incode,"%s\n", tm);
      Replace(incode,"$argnum",argnum, DOH_REPLACE_ANY);
      Replace(incode,"$arg",source, DOH_REPLACE_ANY);
    }
    /* Check if there was any cleanup code (save it for later) */
    if ((tm = Swig_typemap_lookup((char*)"freearg",pt,pn,target,(char*)"tcl_result",0))) {
       Printf(cleanup,"%s\n", tm);
      Replace(cleanup,"$argnum",argnum, DOH_REPLACE_ANY);
      Replace(cleanup,"$arg",source,DOH_REPLACE_ANY);
    }
    /* Look for output arguments */
    if ((tm = Swig_typemap_lookup((char*)"argout",pt,pn,target,(char*)"tcl_result",0))) {
      Printf(outarg,"%s\n", tm);
      Replace(outarg,"$argnum",argnum, DOH_REPLACE_ANY);
      Replace(outarg,"$arg",source, DOH_REPLACE_ANY);
    }
    i++;
    p = Getnext(p);
  }

  Printf(argstr,":%s\"",usage_string(iname,d,l));
  Printv(f,
	 "if (SWIG_GetArgs(interp, objc, objv,", argstr, args, ") == TCL_ERROR) return TCL_ERROR;\n",
	 0);

  Printv(f,incode,0);

  /* Now write code to make the function call */
  emit_func_call(node,f);

  /* Return value if necessary  */
  if ((tm = Swig_typemap_lookup((char*)"out",d,name,(char*)"result",(char*)"tcl_result",0))) {
    Printf(f,"%s\n", tm);
  } else {
    switch(SwigType_type(d)) {
      case T_BOOL:
      case T_INT:
      case T_SHORT:
      case T_LONG :
      case T_SCHAR:
      case T_UINT:
      case T_USHORT:
      case T_ULONG:
      case T_UCHAR:
	Printv(f, "Tcl_SetObjResult(interp,Tcl_NewIntObj((long) result));\n",0);
	break;

	/* Is a single character.  We return it as a string */
      case T_CHAR :
	Printv(f, "Tcl_SetObjResult(interp,Tcl_NewStringObj(&result,1));\n",0);
	break;

      case T_DOUBLE :
      case T_FLOAT :
	Printv(f, "Tcl_SetObjResult(interp,Tcl_NewDoubleObj((double) result));\n",0);
	break;

      case T_USER :

	/* Okay. We're returning malloced memory at this point.
	   Probably dangerous, but safe programming is for wimps. */
	SwigType_add_pointer(d);
	SwigType_remember(d);
	Printv(f, "Tcl_SetObjResult(interp,SWIG_NewPointerObj((void *) result,SWIGTYPE",
	       SwigType_manglestr(d), "));\n", 0);

	SwigType_del_pointer(d);
	break;

    case T_STRING:
	Printv(f, "Tcl_SetObjResult(interp,Tcl_NewStringObj(result,-1));\n",0);
	break;
    case T_POINTER: case T_REFERENCE: case T_ARRAY:
	SwigType_remember(d);
	Printv(f, "Tcl_SetObjResult(interp,SWIG_NewPointerObj((void *) result,SWIGTYPE",
	       SwigType_manglestr(d), "));\n",
	       0);
	break;

    case T_VOID:
      break;

    default :
      Printf(stderr,"%s:%d. Unable to use return type %s in function %s.\n",
	     Getfile(node), Getline(node), SwigType_str(d,0), name);
      break;
    }
  }

  /* Dump output argument code */
  Printv(f,outarg,0);

  /* Dump the argument cleanup code */
  Printv(f,cleanup,0);

  /* Look for any remaining cleanup */
  if (NewObject) {
    if ((tm = Swig_typemap_lookup((char*)"newfree",d,iname,(char*)"result",(char*)"",0))) {
      Printf(f,"%s\n", tm);
    }
  }

  if ((tm = Swig_typemap_lookup((char*)"ret",d,name,(char*)"result",(char*)"",0))) {
    Printf(f,"%s\n", tm);
  }
  Printv(f, "return TCL_OK;\n}\n", 0);

  /* Substitute the cleanup code */
  Replace(f,"$cleanup",cleanup,DOH_REPLACE_ANY);
  Replace(f,"$name", iname, DOH_REPLACE_ANY);

  /* Dump out the function */
  Printf(f_wrappers,"%s",f);

  /* Register the function with Tcl */
  Printv(cmd_info, tab4, "{ SWIG_prefix \"", iname, "\", ", Swig_name_wrapper(iname), ", NULL},\n", 0);

  Delete(incode);
  Delete(cleanup);
  Delete(outarg);
  Delete(argstr);
  Delete(args);
  Delete(f);
}

/* -----------------------------------------------------------------------------
 * TCL8::variable()
 * ----------------------------------------------------------------------------- */

static Hash      *setf = 0;
static Hash      *getf = 0;

void
TCL8::variable(DOH *node) {
  char *name, *iname;
  SwigType *t;

  String *setname;
  String *getname;

  int isarray = 0;
  int readonly = 0;
  int setable = 1;
  int tc;

  name = GetChar(node,"name");
  iname = GetChar(node,"scriptname");
  t = Getattr(node,"type");

  if (!setf) setf = NewHash();
  if (!getf) getf = NewHash();

  /* See if there were any typemaps */
  if (Swig_typemap_search((char *)"varin",t,name) || (Swig_typemap_search((char*)"varout",t,name))) {
    Printf(stderr,"%s:%d. Warning. varin/varout typemap methods not supported.",
	    Getfile(node), Getline(node));
  }

  if (ReadOnly) readonly = 1;
  isarray = SwigType_isarray(t);
  tc = SwigType_type(t);
  setname = Getattr(setf,t);
  getname = Getattr(getf,t);

  /* Dump a collection of set/get functions suitable for variable tracing */
  if (!getname) {
    Wrapper *get, *set;

    setname = NewStringf("swig_%s_set", Swig_string_mangle(t));
    getname = NewStringf("swig_%s_get", Swig_string_mangle(t));
    get = NewWrapper();
    set = NewWrapper();
    Printv(set, "static char *", setname, "(ClientData clientData, Tcl_Interp *interp, char *name1, char *name2, int flags) {\n",0);
    Printv(get, "static char *", getname, "(ClientData clientData, Tcl_Interp *interp, char *name1, char *name2, int flags) {\n",0);

    SwigType *lt = Swig_clocal_type(t);
    if ((tc != T_USER) && (!isarray))
      SwigType_add_pointer(lt);
    Wrapper_add_localv(get,"addr",SwigType_lstr(lt,"addr"),0);
    Wrapper_add_localv(set,"addr",SwigType_lstr(lt,"addr"),0);
    Printv(set, "addr = (", SwigType_lstr(lt,0), ") clientData;\n", 0);
    Printv(get, "addr = (", SwigType_lstr(lt,0), ") clientData;\n", 0);
    if ((tc != T_USER) && (!isarray))
      SwigType_del_pointer(lt);
    Delete(lt);
    Wrapper_add_local(set, "value", "char *value");
    Wrapper_add_local(get, "value", "Tcl_Obj *value");

    Printv(set, "value = Tcl_GetVar2(interp, name1, name2, flags);\n",
	              "if (!value) return NULL;\n", 0);

    switch(tc) {
    case T_INT:
    case T_SHORT:
    case T_USHORT:
    case T_LONG:
    case T_UCHAR:
    case T_SCHAR:
    case T_BOOL:
      Printv(set, "*(addr) = (", SwigType_str(t,0), ") atol(value);\n", 0);
      Wrapper_add_local(get,"value","Tcl_Obj *value");
      Printv(get,
	     "value = Tcl_NewIntObj((int) *addr);\n",
	     "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
	     "Tcl_DecrRefCount(value);\n",
	     0);
      break;

    case T_UINT:
    case T_ULONG:
      Printv(set, "*(addr) = (", SwigType_str(t,0), ") strtoul(value,0,0);\n",0);
      Wrapper_add_local(get,"value","Tcl_Obj *value");
      Printv(get,
	     "value = Tcl_NewIntObj((int) *addr);\n",
	     "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
	     "Tcl_DecrRefCount(value);\n",
	     0);
      break;

    case T_FLOAT:
    case T_DOUBLE:
      Printv(set, "*(addr) = (", SwigType_str(t,0), ") atof(value);\n",0);
      Wrapper_add_local(get,"value","Tcl_Obj *value");
      Printv(get,
	     "value = Tcl_NewDoubleObj((double) *addr);\n",
	     "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
	     "Tcl_DecrRefCount(value);\n",
	     0);
      break;

    case T_CHAR:
      Printv(set, "*(addr) = *value;\n",0);
      Wrapper_add_local(get,"temp", "char temp[2]");
      Printv(get, "temp[0] = *addr; temp[1] = 0;\n",
	     "Tcl_SetVar2(interp,name1,name2,temp,flags);\n",
	     0);
      break;

    case T_USER:
      /* User defined type.  We return it as a pointer */
      SwigType_add_pointer(t);
      SwigType_remember(t);
      Printv(set, "{\n",
	     "void *ptr;\n",
	     "if (SWIG_ConvertPtrFromString(interp,value,&ptr,SWIGTYPE", SwigType_manglestr(t), ") != TCL_OK) {\n",
	     "return \"Type Error\";\n",
	     "}\n",
	     "*(addr) = *((", SwigType_lstr(t,0), ") ptr);\n",
	     "}\n",
	     0);

      SwigType_del_pointer(t);
      Wrapper_add_local(get,"value", "Tcl_Obj *value");
      SwigType_add_pointer(t);
      SwigType_remember(t);
      Printv(get,  "value = SWIG_NewPointerObj(addr, SWIGTYPE", SwigType_manglestr(t), ");\n",
	     "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
	     "Tcl_DecrRefCount(value);\n",0);
      SwigType_del_pointer(t);

      break;

    case T_STRING:
      Printv(set, "if (*addr) free(*addr);\n",
	     "*addr = (char *) malloc(strlen(value)+1);\n",
	     "strcpy(*addr,value);\n",
	     0);
      Printv(get, "Tcl_SetVar2(interp,name1,name2,*addr, flags);\n",0);
      break;

    case T_ARRAY:
      {
	SwigType *aop;
	SwigType *ta = Copy(t);
	aop = SwigType_pop(ta);
	/*	Printf(stdout,"'%s' '%s'\n", ta, aop);*/
	setable = 0;
	readonly = 1;
	if (SwigType_type(ta) == T_CHAR) {
	  String *dim = SwigType_array_getdim(aop,0);
	  if (dim && Len(dim)) {
	    Printf(set, "strncpy(addr,value,%s);\n", dim);
	    setable = 1;
	    readonly = ReadOnly;
	  }
	  Printv(get, "Tcl_SetVar2(interp,name1,name2,addr, flags);\n",0);
	} else {
	  Printf(stderr,"%s:%d: Array variable '%s' will be read-only.\n", Getfile(node), Getline(node), name);
	  Wrapper_add_local(get,"value","Tcl_Obj *value");
	  SwigType_remember(t);
	  Printv(get,
		 "value = SWIG_NewPointerObj(addr, SWIGTYPE", SwigType_manglestr(t), ");\n",
		 "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
		 "Tcl_DecrRefCount(value);\n",
		 0);
	}
	Delete(ta);
	Delete(aop);
      }
      break;

    case T_POINTER: case T_REFERENCE:
      SwigType_remember(t);
      Printv(set,  "{\n",
	     "void *ptr;\n",
	     "if (SWIG_ConvertPtrFromString(interp,value,&ptr,SWIGTYPE", SwigType_manglestr(t), ") != TCL_OK) {\n",
	     "return \"Type Error\";\n",
	     "}\n",
	     "*(addr) = (", SwigType_lstr(t,0), ") ptr;\n",
	     "}\n",
	     0);

      Wrapper_add_local(get,"value","Tcl_Obj *value");
      Printv(get,
	     "value = SWIG_NewPointerObj(*addr, SWIGTYPE", SwigType_manglestr(t), ");\n",
	     "Tcl_SetVar2(interp,name1,name2,Tcl_GetStringFromObj(value,NULL), flags);\n",
	     "Tcl_DecrRefCount(value);\n",
	     0);

      break;
    case T_VOID:
      break;

    default:
      Printf(stderr,"TCL8::link_variable. Unknown type %s!\n", SwigType_str(t,0));
      break;
    }
    Printv(set, "return NULL;\n", "}\n",0);
    Printv(get, "return NULL;\n", "}\n",0);
    Printf(f_wrappers,"%s",get);
    Setattr(getf,Copy(t),getname);
    if (setable) {
      Printf(f_wrappers,"%s",set);
      Setattr(setf,Copy(t),setname);
    }
    Delete(get);
    Delete(set);
  }
  Printv(var_info, tab4,"{ SWIG_prefix \"", iname, "\", (void *) ", isarray ? "" : "&", name, ",", getname, ",", 0);

  if (readonly) {
    static int readonlywrap = 0;
    if (!readonlywrap) {
      Wrapper *ro = NewWrapper();
      Printf(ro, "static char *swig_readonly(ClientData clientData, Tcl_Interp *interp, char *name1, char *name2, int flags) {\n");
      Printv(ro, "return \"Variable is read-only\";\n", "}\n", 0);
      Printf(f_wrappers,"%s",ro);
      readonlywrap = 1;
      Delete(ro);
    }
    Printf(var_info, "swig_readonly},\n");
  } else {
    Printv(var_info, setname, "},\n",0);
  }
}

/* -----------------------------------------------------------------------------
 * TCL8::constant()
 * ----------------------------------------------------------------------------- */

void
TCL8::constant(DOH *node) {
  char   *name;
  SwigType *type;
  char   *value;
  int OldStatus = ReadOnly;
  SwigType *t;
  char      var_name[256];
  char     *tm;
  String   *rvalue;
  ReadOnly = 1;
  DOH      *nnode;

  name = GetChar(node,"name");
  type = Getattr(node,"type");
  value = GetChar(node,"value");
  nnode = Copy(node);

  /* Make a static variable */
  sprintf(var_name,"_wrap_const_%s",name);

  if (SwigType_type(type) == T_STRING) {
    rvalue = NewStringf("\"%s\"",value);
  } else if (SwigType_type(type) == T_CHAR) {
    rvalue = NewStringf("\'%s\'",value);
  } else {
    rvalue = NewString(value);
  }
  if ((tm = Swig_typemap_lookup((char*)"const",type,name,Char(rvalue),name,0))) {
    Printf(f_init,"%s\n",tm);
  } else {
    /* Create variable and assign it a value */
    switch(SwigType_type(type)) {
    case T_BOOL: case T_INT: case T_DOUBLE:
      Printf(f_header,"static %s %s = %s;\n", SwigType_str(type,0), var_name, value);
      Setattr(nnode,"name",var_name);
      variable(nnode);
      break;

    case T_SHORT:
    case T_LONG:
    case T_SCHAR:
      Printf(f_header,"static %s %s = %s;\n", SwigType_str(type,0), var_name, value);
      Printf(f_header,"static char *%s_char;\n", var_name);
      if (CPlusPlus)
	Printf(f_init,"\t %s_char = new char[32];\n",var_name);
      else
	Printf(f_init,"\t %s_char = (char *) malloc(32);\n",var_name);

      Printf(f_init,"\t sprintf(%s_char,\"%%ld\", (long) %s);\n", var_name, var_name);
      sprintf(var_name,"%s_char",var_name);
      t = NewString("char");
      SwigType_add_pointer(t);
      Setattr(nnode,"name",var_name);
      Setattr(nnode,"type",t);
      variable(nnode);
      Delete(t);
      break;

    case T_UINT:
    case T_USHORT:
    case T_ULONG:
    case T_UCHAR:
      Printf(f_header,"static %s %s = %s;\n", SwigType_str(type,0), var_name, value);
      Printf(f_header,"static char *%s_char;\n", var_name);
      if (CPlusPlus)
	Printf(f_init,"\t %s_char = new char[32];\n",var_name);
      else
	Printf(f_init,"\t %s_char = (char *) malloc(32);\n",var_name);

      Printf(f_init,"\t sprintf(%s_char,\"%%lu\", (unsigned long) %s);\n", var_name, var_name);
      sprintf(var_name,"%s_char",var_name);
      t = NewSwigType(T_CHAR);
      SwigType_add_pointer(t);
      Setattr(nnode,"name",var_name);
      Setattr(nnode,"type",t);
      variable(nnode);
      Delete(t);
      break;

    case T_FLOAT:
      Printf(f_header,"static %s %s = (%s) (%s);\n", SwigType_lstr(type,0), var_name, SwigType_lstr(type,0), value);
      Setattr(nnode,"name",var_name);
      variable(nnode);
      break;

    case T_CHAR:
      SwigType_add_pointer(type);
      Printf(f_header,"static %s %s = \"%s\";\n", SwigType_lstr(type,0), var_name, value);
      Setattr(nnode,"name",var_name);
      Setattr(nnode,"type",type);
      variable(nnode);
      SwigType_del_pointer(type);
      break;

    case T_STRING:
      Printf(f_header,"static %s %s = \"%s\";\n", SwigType_lstr(type,0), var_name, value);
      Setattr(nnode,"name",var_name);
      variable(nnode);
      break;

    case T_POINTER: case T_ARRAY: case T_REFERENCE:
      Printf(f_header,"static %s = %s;\n", SwigType_lstr(type,var_name), value);
      Printf(f_header,"static char *%s_char;\n", var_name);
      if (CPlusPlus)
	Printf(f_init,"\t %s_char = new char[%d];\n",var_name,(int) Len(SwigType_manglestr(type))+ 20);
      else
	Printf(f_init,"\t %s_char = (char *) malloc(%d);\n",var_name, (int) Len(SwigType_manglestr(type))+ 20);

      t = NewSwigType(T_CHAR);
      SwigType_add_pointer(t);
      SwigType_remember(type);
      Printf(f_init,"\t SWIG_MakePtr(%s_char, (void *) %s, SWIGTYPE%s);\n",
	     var_name, var_name, SwigType_manglestr(type));
      sprintf(var_name,"%s_char",var_name);
      Setattr(nnode,"type",t);
      Setattr(nnode,"name",var_name);
      variable(nnode);
      Delete(t);
      break;

    default:
      Printf(stderr,"%s:%d. Unsupported constant value.\n", Getfile(node), Getline(node));
      break;
    }
  }
  Delete(rvalue);
  Delete(nnode);
  ReadOnly = OldStatus;
}

/* -----------------------------------------------------------------------------
 * TCL8::usage_string()
 * ----------------------------------------------------------------------------- */

char *
TCL8::usage_string(char *iname, SwigType *, ParmList *l) {
  static String *temp = 0;
  Parm  *p;
  int   i, numopt,pcount;

  if (!temp) temp = NewString("");
  Clear(temp);
  if (nspace) {
    Printf(temp,"%s::%s", ns_name,iname);
  } else {
    Printf(temp,"%s ", iname);
  }

  /* Now go through and print parameters */
  i = 0;
  pcount = ParmList_len(l);
  numopt = check_numopt(l);
  for (p = l; p; p = Getnext(p)) {
    SwigType   *pt = Gettype(p);
    String  *pn = Getname(p);

    /* Only print an argument if not ignored */
    if (!Swig_typemap_search((char*)"ignore",pt,pn)) {
      if (i >= (pcount-numopt))
	Putc('?',temp);
      /* If parameter has been named, use that.   Otherwise, just print a type  */
      if (SwigType_type(pt) != T_VOID) {
	if (Len(pn) > 0) {
	  Printf(temp, "%s",pn);
	} else {
	  Printf(temp,"%s", SwigType_str(pt,0));
	}
      }
      if (i >= (pcount-numopt))	Putc('?',temp);
      Putc(' ',temp);
      i++;
    }
  }
  return Char(temp);
}

/* -----------------------------------------------------------------------------
 * TCL8::nativefunction();
 * ----------------------------------------------------------------------------- */

void
TCL8::nativefunction(DOH *node) {
  char *name;
  char *funcname;

  name = GetChar(node,"scriptname");
  funcname = GetChar(node,"name");
  Printf(f_init,"\t Tcl_CreateObjCommand(interp, SWIG_prefix \"%s\", %s, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);\n",name, funcname);
}

/* -----------------------------------------------------------------------------
 * C++ Handling
 * ----------------------------------------------------------------------------- */

void
TCL8::cpp_open_class(DOH *node) {
  this->Language::cpp_open_class(node);

  char *classname = GetChar(node,"name");
  char *rename = GetChar(node,"scriptname");
  char *ctype = GetChar(node,"classtype");
  int   strip = GetInt(node,"strip");
  
  if (shadow) {
    static int included_object = 0;
    if (!included_object) {
      if (Swig_insert_file("object.swg",f_header) == -1) {
	Printf(stderr,"SWIG : Fatal error. Unable to locate 'object.swg' in SWIG library.\n");
	Swig_exit (EXIT_FAILURE);
      }
      included_object = 1;
    }

    Clear(attributes);
    Printf(attributes, "static swig_attribute swig_");
    Printv(attributes, classname, "_attributes[] = {\n", 0);

    Clear(methods);
    Printf(methods,"static swig_method swig_");
    Printv(methods, classname, "_methods[] = {\n", 0);

    have_constructor = 0;
    have_destructor = 0;

    Delete(class_name);
    Delete(class_type);
    Delete(real_classname);

    class_name = rename ? NewString(rename) : NewString(classname);
    class_type = strip  ? NewString("") : NewStringf("%s ",ctype);
    real_classname = NewString(classname);
  }
}

void
TCL8::cpp_close_class() {
  SwigType *t;
  String *code = NewString("");

  this->Language::cpp_close_class();
  if (shadow) {
    t = NewStringf("%s%s", class_type, real_classname);
    SwigType_add_pointer(t);

    if (have_destructor) {
      Printv(code, "static void swig_delete_", class_name, "(void *obj) {\n", 0);
      if (CPlusPlus) {
	Printv(code,"    delete (", SwigType_str(t,0), ") obj;\n",0);
      } else {
	Printv(code,"    free((char *) obj);\n",0);
      }
      Printf(code,"}\n");
    }

    Printf(methods, "    {0,0}\n};\n");
    Printv(code,methods,0);

    Printf(attributes, "    {0,0,0}\n};\n");
    Printv(code,attributes,0);

    Printv(code, "static swig_class _wrap_class_", class_name, " = { \"", class_name,
	   "\", &SWIGTYPE", SwigType_manglestr(t), ",",0);

    if (have_constructor) {
      Printf(code, "%s", Swig_name_wrapper(Swig_name_construct(class_name)));
    } else {
      Printf(code,"0");
    }
    if (have_destructor) {
      Printv(code, ", swig_delete_", class_name,0);
    } else {
      Printf(code,",0");
    }
    Printv(code, ", swig_", real_classname, "_methods, swig_", real_classname, "_attributes };\n", 0);
    Printf(f_wrappers,"%s",code);

    Printv(cmd_info, tab4, "{ SWIG_prefix \"", class_name, "\", SwigObjectCmd, &_wrap_class_", class_name, "},\n", 0);
  }
  Delete(code);
}

void TCL8::cpp_memberfunction(DOH *node) {
  char *name, *iname;
  char *realname;
  char temp[1024];
  String  *rname;
  
  this->Language::cpp_memberfunction(node);
  if (shadow) {
    name = GetChar(node,"name");
    iname = GetChar(node,"scriptname");
    realname = iname ? iname : name;
    /* Add stubs for this member to our class handler function */

    strcpy(temp, Char(Swig_name_member(class_name,realname)));
    rname = Getattr(repeatcmd,temp);
    if (!rname) rname = Swig_name_wrapper(temp);

    Printv(methods, tab4, "{\"", realname, "\", ", rname, "}, \n", 0);
  }
}

void TCL8::cpp_variable(DOH *node) {
  char *name, *iname;
  char *realname;
  char temp[1024];
  String *rname;

  this->Language::cpp_variable(node);

  if (shadow) {
    name = GetChar(node,"name");
    iname = GetChar(node,"scriptname");
    realname = iname ? iname : name;
    Printv(attributes, tab4, "{ \"-", realname, "\",", 0);

    /* Try to figure out if there is a wrapper for this function */
    strcpy(temp, Char(Swig_name_get(Swig_name_member(class_name,realname))));
    rname = Getattr(repeatcmd,temp);
    if (!rname) rname = Swig_name_wrapper(temp);
    Printv(attributes, rname, ", ", 0);

    if (!ReadOnly) {
      strcpy(temp, Char(Swig_name_set(Swig_name_member(class_name,realname))));
      rname = Getattr(repeatcmd,temp);
      if (!rname) rname = Swig_name_wrapper(temp);
      Printv(attributes, rname, "},\n",0);
    } else {
      Printf(attributes, "0 },\n");
    }
  }
}

void
TCL8::cpp_constructor(DOH *node) {
  this->Language::cpp_constructor(node);
  have_constructor = 1;
}

void
TCL8::cpp_destructor(DOH *node) {
  this->Language::cpp_destructor(node);
  have_destructor = 1;
}
