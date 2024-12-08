module;

export module vkl_platform:console;

import std;
import std.compat;

export namespace vkl
{
	void attach_console()
	{
		if (not GetStdHandle(STD_OUTPUT_HANDLE))
		{
			if (AttachConsole(ATTACH_PARENT_PROCESS))
			{
				freopen("CONOUT$", "wb", stdout);
				freopen("CONOUT$", "wb", stderr);
			}
		}
	}
}