#pragma once

#include"re3d/base/def.h"
#include<string>

namespace ff {
	class ArgSet;
}


using ff::ArgSet;
using std::string;

_RE3D_BEG


class _RE3D_API Re3DCommand
{
public:
	enum
	{
		FUNCT_APP0 = 10, //should be the same as vfxEHT_APICMD_APP1
		FUNCT_APP1
	};

	static int getFuncType(void(*)(ArgSet&))
	{
		return FUNCT_APP1;
	}
	static int getFuncType(void(*)())
	{
		return FUNCT_APP0;
	}

	class _RE3D_API Command
	{
	public:
		const char*			  m_cmd;			//related command, empty if not used
		void*				  m_func;           //the callback function
		int					  m_funcType;       //the type of @m_func
		const char*			  m_defArgStr;      //default command-line argument
		const char*			  m_shortHelp;      //a short help
		const char*           m_example;        //example usages
		void				 *m_pCustom;
	public:
		Command(const char *cmd = "", void *func = nullptr, int funcType = 0, const char *defArg = "", const char *shortHelp = "", const char *example="", void *pCustom = NULL)
			:m_cmd(cmd), m_func(func), m_funcType(funcType), m_defArgStr(defArg), m_shortHelp(shortHelp), m_example(example), m_pCustom(pCustom)
		{
		}
	};

	class _RE3D_API CommandList
	{
	public:
		const Command   *m_cmds;
		int			     m_count;
		CommandList		*m_pNext;
	public:
		CommandList(const Command *cmds, int count);
	};

	// static CommandList *g_cmdList;
};

//#define _Re3DAPI_UN_TAG(tag) tag
//#define _Re3DAPI_UN_ID()   __LINE__


#define Re3D_ADD_COMMANDS(cmdsTab) \
	static re3d::Re3DCommand::CommandList _Re3D_UNIQUE_NAME(_Re3D_auto_obj_add_commands)(cmdsTab,sizeof(cmdsTab)/sizeof(cmdsTab[0]));

#define CMD_BEG() namespace _Re3D_UNIQUE_NAME(_Re3Dcmd) { static re3d::Re3DCommand::Command cmdtab[]={

#define CMD(cmd, func, defArg, shortHelp, example) re3d::Re3DCommand::Command(cmd,func,re3d::Re3DCommand::getFuncType(func),defArg,shortHelp,example,nullptr),
#define CMD0(cmd,func) CMD(cmd,func,"","","")

#define CMD_END() }; Re3D_ADD_COMMANDS(cmdtab); }



_RE3D_END

