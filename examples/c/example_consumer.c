#include <example.h>
#include <std_string.h>
#include <stdio.h>

static void print_vec(const char *label, const Example_Vector3 *v)
{
    Example_std_string *s = Example_Vector3_ToString(v);
    printf("%s = %s\n", label, Example_std_string_data(s));
    Example_std_string_Destroy(s);
}

int main(void)
{
    Example_SayHello();
    printf("Square(7) = %d\n", Example_Square(7));

    /* Dog */
    Example_Dog *dog = Example_Dog_Construct("Rex", NULL, 3, "Labrador", NULL);

    Example_std_string *dog_sound = Example_Dog_Speak(dog);
    printf("%s\n", Example_std_string_data(dog_sound));
    Example_std_string_Destroy(dog_sound);

    Example_Dog_Fetch(dog, "ball", NULL);

    /* Cat (pass NULL for indoor to use the default argument = true) */
    Example_Cat *cat = Example_Cat_Construct("Whiskers", NULL, 5, NULL);

    Example_std_string *cat_sound = Example_Cat_Speak(cat);
    printf("%s\n", Example_std_string_data(cat_sound));
    Example_std_string_Destroy(cat_sound);

    Example_Cat_Purr(cat);

    /* Polymorphism via base-class pointer */
    const Example_Animal *animal = Example_Dog_UpcastTo_Example_Animal(dog);
    Example_std_string *poly_sound = Example_MakeAnimalSpeak(animal);
    printf("Via base: %s\n", Example_std_string_data(poly_sound));
    Example_std_string_Destroy(poly_sound);

    Example_Dog_Destroy(dog);
    Example_Cat_Destroy(cat);

    /* Vector3 operators */
    Example_Vector3 *a = Example_Vector3_Construct(1.0f, 2.0f, 3.0f);
    Example_Vector3 *b = Example_Vector3_Construct(4.0f, 5.0f, 6.0f);

    print_vec("a", a);
    print_vec("b", b);

    Example_Vector3 *sum    = Example_add_Example_Vector3(a, b);
    Example_Vector3 *diff   = Example_sub_Example_Vector3(a, b);
    Example_Vector3 *scaled = Example_mul_Example_Vector3_float(a, 3.0f);
    Example_Vector3 *halved = Example_div_Example_Vector3_float(a, 2.0f);

    print_vec("a + b", sum);
    print_vec("a - b", diff);
    print_vec("a * 3", scaled);
    print_vec("a / 2", halved);

    printf("a . b = %f\n", Example_Vector3_Dot(a, b));
    printf("|a|^2 = %f\n", Example_Vector3_LengthSquared(a));

    Example_Vector3 *cross = Example_Vector3_Cross(a, b);
    print_vec("a x b", cross);

    printf("a == b: %d\n", Example_equal_Example_Vector3(a, b));
    printf("a != b: %d\n", Example_not_equal_Example_Vector3(a, b));

    Example_Vector3 *a2 = Example_Vector3_Construct(1.0f, 2.0f, 3.0f);
    printf("a == a2: %d\n", Example_equal_Example_Vector3(a, a2));

    /* compound assignment: a2 += b */
    Example_Vector3_add_assign(a2, b);
    print_vec("a2 += b", a2);

    Example_Vector3_Destroy(a);
    Example_Vector3_Destroy(b);
    Example_Vector3_Destroy(a2);
    Example_Vector3_Destroy(sum);
    Example_Vector3_Destroy(diff);
    Example_Vector3_Destroy(scaled);
    Example_Vector3_Destroy(halved);
    Example_Vector3_Destroy(cross);

    return 0;
}
