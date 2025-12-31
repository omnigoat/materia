using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Mantle.Commands;

internal static class Gen
{
	public static void Execute(System.CommandLine.ParseResult pr, string target, string arch, string config, string ide)
	{
		// 1. go directly to named package

		// 2. recursively walk dependencies, accumulating packages

		// 3. generate to output directory

	}
}
