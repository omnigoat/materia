using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System.Reflection;

namespace Mantle;

public class Dyncom
{
	static string code = @"
			using System;

			namespace DynamicCode
			{
				public class MyDynamicClass
				{
					public string GetMessage()
					{
						Console.WriteLine(""Hello from dynamic code! "");
						Mantle.Dyncom.Execute();
						return """";
					}
				}
			}";

	public static int Execute()
	{
		SyntaxTree syntaxTree = CSharpSyntaxTree.ParseText(code);

		string assemblyPath = Path.GetDirectoryName(typeof(object).Assembly.Location) ?? "";

		var hmm = typeof(Mantle).Assembly.GetReferencedAssemblies();

		CSharpCompilation compilation = CSharpCompilation.Create(
			"DynamicAssembly",
			[syntaxTree],
			[
				MetadataReference.CreateFromFile(typeof(Mantle).Assembly.Location),
				MetadataReference.CreateFromFile(typeof(object).Assembly.Location),
				MetadataReference.CreateFromFile(Path.Combine(assemblyPath, "System.dll")),
				MetadataReference.CreateFromFile(Path.Combine(assemblyPath, "System.Core.dll")),
				MetadataReference.CreateFromFile(Path.Combine(assemblyPath, "System.Runtime.dll")),
				MetadataReference.CreateFromFile(Path.Combine(assemblyPath, "System.Console.dll")),
			],
			new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));

		compilation.Emit("mything.dll");

		using (var ms = new MemoryStream())
		{
			var result = compilation.Emit(ms);
			if (result.Success)
			{
				ms.Seek(0, SeekOrigin.Begin);
				// 3. Load the compiled assembly from memory
				Assembly dynamicAssembly = Assembly.Load(ms.ToArray());

				// 4. Execute code from the loaded assembly using reflection
				Type? myClassType = dynamicAssembly.GetType("DynamicCode.MyDynamicClass");
				if (myClassType != null)
				{
					object? instance = Activator.CreateInstance(myClassType);
					MethodInfo? method = myClassType.GetMethod("GetMessage");
					if (method != null)
					{
						//string? resultValue = (string?)method.Invoke(instance, []);
						//Console.WriteLine(resultValue);
						method.Invoke(instance, []);
					}
				}
			}
			else
			{
				foreach (var diagnostic in result.Diagnostics)
				{
					Console.WriteLine(diagnostic.ToString());
				}
			}
		}

		return 0;
	}
}
