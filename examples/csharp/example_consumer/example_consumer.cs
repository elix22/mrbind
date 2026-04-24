class Program
{
    static void PrintVec(string label, Example.Const_Vector3 v)
    {
        using var s = v.ToString();
        Console.WriteLine($"{label} = {s}");
    }

    static void Main()
    {
        Example.SayHello();
        Console.WriteLine($"Square(7) = {Example.Square(7)}");

        // Dog
        using var dog = new Example.Dog("Rex", 3, "Labrador");
        using var dogSound = dog.Speak();
        Console.WriteLine((string)dogSound);
        dog.Fetch("ball");

        // Cat (indoor defaults to true when null)
        using var cat = new Example.Cat("Whiskers", 5);
        using var catSound = cat.Speak();
        Console.WriteLine((string)catSound);
        cat.Purr();

        // Polymorphism via upcast to Animal (cast to Const_Dog first to resolve ambiguity)
        Example.Const_Animal animal = (Example.Const_Dog)dog;
        using var polySound = Example.MakeAnimalSpeak(animal);
        Console.WriteLine($"Via base: {(string)polySound}");

        // Vector3 operators
        using var a = new Example.Vector3(1f, 2f, 3f);
        using var b = new Example.Vector3(4f, 5f, 6f);

        PrintVec("a", a);
        PrintVec("b", b);

        using var sum    = a + b;
        using var diff   = a - b;
        using var scaled = a * 3f;
        using var halved = a / 2f;

        PrintVec("a + b", sum);
        PrintVec("a - b", diff);
        PrintVec("a * 3", scaled);
        PrintVec("a / 2", halved);

        Console.WriteLine($"a . b = {a.Dot(b)}");
        Console.WriteLine($"|a|^2 = {a.LengthSquared()}");

        using var cross = a.Cross(b);
        PrintVec("a x b", cross);

        Console.WriteLine($"a == b: {a == b}");
        Console.WriteLine($"a != b: {a != b}");

        using var a2 = new Example.Vector3(1f, 2f, 3f);
        Console.WriteLine($"a == a2: {a == a2}");

        // Compound assignment
        a2.AddAssign(b);
        PrintVec("a2 += b", a2);
    }
}
