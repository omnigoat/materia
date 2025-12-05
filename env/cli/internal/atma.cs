using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using System.Reflection;
using System.IO;
using System.Runtime.Loader;

string codeToCompile = @"
        using System;
        namespace DynamicCode
        {
            public class MyDynamicClass
            {
                public string GetMessage()
                {
                    return ""Hello from dynamically compiled code!"";
                }
            }
        }";


 // Create a syntax tree from the code string
SyntaxTree syntaxTree = CSharpSyntaxTree.ParseText(codeToCompile);

// Define compilation options (e.g., output type, references)
CSharpCompilationOptions options = new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary);

// Get references to necessary assemblies (e.g., System.Runtime)
// You'll need to add references to any assemblies your dynamic code depends on.
MetadataReference[] references = new MetadataReference[]
{
    MetadataReference.CreateFromFile(typeof(object).Assembly.Location)
};

// Create a CSharpCompilation object
CSharpCompilation compilation = CSharpCompilation.Create(
    "DynamicAssembly",
    new[] { syntaxTree },
    references,
    options);

// Compile the code into a memory stream
using (var ms = new MemoryStream())
{
    var result = compilation.Emit(ms);

    if (result.Success)
    {
        ms.Seek(0, SeekOrigin.Begin); // Rewind the stream
        // Load the compiled assembly from the memory stream
        Assembly assembly = AssemblyLoadContext.Default.LoadFromStream(ms);

        // Get the type and create an instance
        Type dynamicType = assembly.GetType("DynamicCode.MyDynamicClass");
        object instance = Activator.CreateInstance(dynamicType);

        // Invoke a method on the dynamic instance
        MethodInfo method = dynamicType.GetMethod("GetMessage");
        string message = (string)method.Invoke(instance, null);
        Console.WriteLine(message); // Output: Hello from dynamically compiled code!
    }
    else
    {
        foreach (var diagnostic in result.Diagnostics)
        {
            Console.WriteLine(diagnostic.ToString());
        }
    }
}

#if false
class Bam
{
    static void Main(string[] args)
    {
        Console.WriteLine("hooray2");
        foreach (var arg in args)
        {
            Console.WriteLine($"  arg: {arg}");
        }
    }
}
#endif