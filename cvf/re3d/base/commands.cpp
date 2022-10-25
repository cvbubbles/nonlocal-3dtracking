
#include"commands.h"
#include"BFC/argv.h"

#include"BFC/stdf.h"
#include<experimental/filesystem>
#include<string.h>

#ifndef _MSC_VER
#define strnicmp strncasecmp
#endif 


_RE3D_BEG

static Re3DCommand::CommandList *g_cmdList = NULL;

Re3DCommand::CommandList::CommandList(const Re3DCommand::Command *cmds, int count)
	:m_cmds(cmds), m_count(count)
{
	m_pNext = g_cmdList;
	g_cmdList = this;
}

const Re3DCommand::Command* findCommand(const std::string &cmd) {
	auto *p = g_cmdList;
	while (p)
	{
		for (int i = 0; i < p->m_count; ++i)
		{
			if (stricmp(p->m_cmds[i].m_cmd, cmd.c_str()) == 0)
				return &p->m_cmds[i];
		}
		p = p->m_pNext;
	}
	return nullptr;
}

_RE3D_API void exec(const char *cmd)
{
	if (!cmd)
		return;

	const char *end = cmd + strlen(cmd);
	cmd = ff::SkipSpace(cmd, end);
	const char *px = ff::SkipNonSpace(cmd, end);
	if (cmd != px)
	{
		std::string cmdName(cmd, px);
		auto *cmdObj = findCommand(cmdName);
		if (!cmdObj)
		{
			printf("error:can't find command %s\n", cmdName.c_str());
			return;
		}

		if (cmdObj->m_func)
		{
			//try
			{
				if (cmdObj->m_funcType == Re3DCommand::FUNCT_APP1)
				{
					typedef void(*FuncT)(ff::ArgSet &);
					FuncT fp = reinterpret_cast<FuncT>(cmdObj->m_func);

					ff::ArgSet args, defArgs;
					args.setArgs(px);
					if (cmdObj->m_defArgStr)
					{
						defArgs.setArgs(cmdObj->m_defArgStr);
						args.setNext(&defArgs);
					}
					fp(args);
				}
				else if (cmdObj->m_funcType == Re3DCommand::FUNCT_APP0)
				{
					typedef void(*FuncT)();
					FuncT fp = reinterpret_cast<FuncT>(cmdObj->m_func);
					fp();
				}
				else
					printf("warning: unknown func-type of command");
			}
			/*catch (const std::exception &ec)
			{
				printf("exception: %s\n", ec.what());
				throw;
			}
			catch (...)
			{
				printf("exception: unknown error\n");
			}*/
		}
	}
}

static void on_list(ArgSet &args)
{
	std::string name = args.getd<string>("#0", "");

	auto *p = g_cmdList;
	while (p)
	{
		for (int i = 0; i < p->m_count; ++i)
		{
			if (name.empty()|| strnicmp(p->m_cmds[i].m_cmd, name.c_str(), name.size()) == 0)
			{
				auto &cmd = p->m_cmds[i];
				printf("#%s : %s\ne.g. %s\n", cmd.m_cmd, cmd.m_shortHelp, cmd.m_example);
			}
		}
		p = p->m_pNext;
	}
}

CMD_BEG()
CMD("list", on_list, "", "list available commands", "list md. (list all commands start with md.)")
CMD_END()

_RE3D_END

