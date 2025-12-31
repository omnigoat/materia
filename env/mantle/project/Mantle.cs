using System.CommandLine;
using System.Linq;

namespace Mantle;

internal static class EnvVars
{
	public static string IdeGenArch = "JIVE_idegen_arch";
	public static string IdeGenConfig = "JIVE_idegen_config";
};

public class Mantle
{
	internal static Dictionary<string, string> defaults = new();
	internal static string jive_path = "jive_defaults.txt";

	private static void ParseDefaultFile()
	{
		if (!File.Exists(jive_path))
			return;

		string[] lines = File.ReadAllLines("jive_defaults.txt");
		foreach (var line in lines)
		{
			string[] split = line.Split(' ');
			if (split.Length == 2)
				defaults.Add(split[0], split[1]);
			else
				Console.WriteLine($"Error at line '{line}'");
		}
	}

	static Action<ParseResult> UnwrapExecutor(Action<ParseResult, string[]> exe, params string[] args)
	{
		return (ParseResult pr) =>
		{
			var transformed = args
				.Select(x => pr.GetValue<string>(x) ?? "")
				.ToArray();

			exe(pr, transformed);
		};
	}

	public static int Main(string[] args)
	{
		ParseDefaultFile();

		defaults.TryGetValue(EnvVars.IdeGenArch, out string? idegen_arch);
		defaults.TryGetValue(EnvVars.IdeGenConfig, out string? idegen_config);

		Command gen = new("gen", "Generates ide files for a target")
		{
			new Argument<string>("target"),
			new Argument<string?>("arch") { DefaultValueFactory = x => idegen_arch ?? "" },
			new Argument<string?>("config") { DefaultValueFactory = x => idegen_config ?? "" },
			new Argument<string>("ide") { DefaultValueFactory = pr => "msvs22" }
		};

		gen.SetAction(pr =>
		{
			//Console.WriteLine($"target: {pr.GetValue<string>("target")}");
			//Console.WriteLine($"arch: {pr.GetValue<string>("arch")}");
			//Console.WriteLine($"config: {pr.GetValue<string>("config")}");
			//Console.WriteLine($"ide-type: {pr.GetValue<string>("ide-type")}");

			var target = pr.GetValue<string>("target") ?? "";
			var arch = pr.GetValue<string>("arch") ?? "";
			var config = pr.GetValue<string>("config") ?? "";
			var ide = pr.GetValue<string>("ide") ?? "";

			Commands.Gen(pr, target, arch, config, ide);
		});

		Command setvar = new("set-default")
		{
			new Argument<string>("varname"),
			new Argument<string>("value")
		};

		setvar.SetAction(pr =>
		{
			string? varname = pr.GetValue<string?>("varname");
			string? value = pr.GetValue<string>("value");
			if (varname != null && value != null)
			{
				if (defaults.ContainsKey(varname))
					defaults[varname] = value;
				else
					defaults.Add(varname, value);
			}

			var lines = string.Join('\n', defaults
				.AsEnumerable()
				.Select(x => $"{x.Key} {x.Value}"));

			File.WriteAllText("jive_defaults.txt", lines);
		});

		RootCommand r = new();
		r.Subcommands.Add(gen);
		r.Subcommands.Add(setvar);

		ParseResult pr = r.Parse(args);
		pr.Invoke();

		return 0;
		//return Dyncom.Execute();
	}
}
