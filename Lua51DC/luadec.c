#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"

#ifndef LUA_DEBUG
#define luaB_opentests(L)
#endif

#ifndef PROGNAME
#define PROGNAME	"luadec"			/* program name */
#endif

#define	OUTPUT		"luadec.out"		/* default output file */

static int debugging = 0;				/* debug decompiler? */
static int functions = 0;				/* dump functions separately? */
static int dumping = 1;					/* dump bytecodes? */
static int stripping = 0;				/* strip debug information? */
static char Output[] = { OUTPUT };		/* default output file name */
static const char* output = Output;		/* output file name */
static const char* progname = PROGNAME;	/* actual program name */

void luaU_decompile(const Proto* f, int lflag, const char* filename);
void luaU_decompileFunctions(const Proto* f, int lflag, const char** filename);

static void fatal(const char* message)
{
	fprintf(stderr, "%s: %s\n", progname, message);
	exit(EXIT_FAILURE);
}

static void usage(const char* message, const char* arg)
{
	if (message != NULL)
	{
		fprintf(stderr, "%s: ", progname); fprintf(stderr, message, arg); fprintf(stderr, "\n");
	}
	fprintf(stderr,
		"usage: %s [options] [filename].  Available options are:\n"
		"  -        process stdin\n"
		"  -d       output information for debugging the decompiler\n"
		"  -f       decompile multiple files ("PROGNAME" file1 file2 file3...)\n"
		"  -o       sets the output file name (default: '"OUTPUT"')\n"
		"  -p       only parse file for errors\n"
		"  -s       strips debug info\n"
		"  -v       shows "PROGNAME" version\n"
		"  --       stop handling options\n",
		progname);
	exit(EXIT_FAILURE);
}

#define	IS(s)	(strcmp(argv[i],s)==0)

static int doargs(int argc, char* argv[])
{
	int i;
	if (argv[0] != NULL && *argv[0] != 0) progname = argv[0];
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')		/* end of options; keep it */
			break;
		else if (IS("--"))			/* end of options; skip it */
		{
			++i;
			break;
		}
		else if (IS("-"))			/* end of options; use stdin */
			return i;
		else if (IS("-d"))			/* list */
			debugging = 1;
		else if (IS("-f"))			/* list */
			functions = 1;
		else if (IS("-o"))			/* output file */
		{
			output = argv[++i];
			if (output == NULL || *output == 0) usage("'-o' needs argument", NULL);
		}
		else if (IS("-p"))			/* parse only */
			dumping = 0;
		else if (IS("-s"))			/* strip debug information */
			stripping = 1;
		else if (IS("-v"))			/* show version */
		{
			printf(PROGNAME" 0.1\n");
			if (argc == 2) exit(EXIT_SUCCESS);
		}
		else						/* unknown option */
			usage("unrecognized option '%s'", argv[i]);
	}
	if (i == argc && (debugging || !dumping))
	{
		dumping = 0;
		argv[--i] = Output;
	}
	return i;
}

static Proto* toproto(lua_State* L, int i)
{
	const Closure* c = (const Closure*)lua_topointer(L, i);
	return c->l.p;
}

static Proto* combine(lua_State* L, int n)
{
	if (n == 1)
		return toproto(L, -1);
	else
	{
		int i, pc = 0;
		Proto* f = luaF_newproto(L);
		f->source = luaS_newliteral(L, "=(" PROGNAME ")");
		f->maxstacksize = 1;
		f->p = luaM_newvector(L, n, Proto*);
		f->sizep = n;
		f->sizecode = 2 * n + 1;
		f->code = luaM_newvector(L, f->sizecode, Instruction);
		for (i = 0; i < n; i++)
		{
			f->p[i] = toproto(L, i - n);
			f->code[pc++] = CREATE_ABx(OP_CLOSURE, 0, i);
			f->code[pc++] = CREATE_ABC(OP_CALL, 0, 1, 1);
		}
		f->code[pc++] = CREATE_ABC(OP_RETURN, 0, 1, 0);
		return f;
	}
}

static void strip(lua_State* L, Proto* f)
{
	int i, n = f->sizep;
	luaM_freearray(L, f->lineinfo, f->sizelineinfo, int);
	luaM_freearray(L, f->locvars, f->sizelocvars, struct LocVar);
	luaM_freearray(L, f->upvalues, f->sizeupvalues, TString*);
	f->lineinfo = NULL; f->sizelineinfo = 0;
	f->locvars = NULL;  f->sizelocvars = 0;
	f->upvalues = NULL; f->sizeupvalues = 0;
	f->source = luaS_newliteral(L, "=(none)");
	for (i = 0; i < n; i++) strip(L, f->p[i]);
}

int main(int argc, char* argv[])
{
	lua_State* L;
	Proto* f;
	int i = doargs(argc, argv);
	argc -= i; argv += i;
	if (argc <= 0) usage("no input files given", NULL);
	L = lua_open();
	luaB_opentests(L);

	const char* filename[256] = {0};
	for (i = 0; i < argc; i++)
	{
		filename[i] = IS("-") ? NULL : argv[i];
		if (luaL_loadfile(L, filename[i]) != 0) fatal(lua_tostring(L, -1));
	}
	f = combine(L, argc);
	if (functions)
		luaU_decompileFunctions(f, debugging, filename);
	else
		luaU_decompile(f, debugging, filename[0]);
	return 0;
}
