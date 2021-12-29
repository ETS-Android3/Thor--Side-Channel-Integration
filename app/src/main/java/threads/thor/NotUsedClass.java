package threads.thor;

import java.io.File;
import java.io.IOException;
import java.math.BigInteger;
import java.util.Map;
import java.util.Random;
import java.util.Scanner;
import java.util.TreeMap;

public class NotUsedClass {
    public static void notUsedMethod(){
        try {
            File f = new File("ciaFactBook2008.txt");
            Scanner sc;
            sc = new Scanner(f);
            // sc.useDelimiter("[^a-zA-Z']+");
            Map<String, Integer> wordCount = new TreeMap<String, Integer>();
            while(sc.hasNext()) {
                String word = sc.next();
                if(!wordCount.containsKey(word))
                    wordCount.put(word, 1);
                else
                    wordCount.put(word, wordCount.get(word) + 1);
            }

            // show results
            for(String word : wordCount.keySet())
                System.out.println(word + " " + wordCount.get(word));
            System.out.println(wordCount.size());
        }
        catch(IOException e) {
            System.out.println("Unable to read from file.");
        }

        printTest(10, 4);
        printTest(2, 2);
        printTest(54161329, 4);
        printTest(1882341361, 2);
        printTest(36, 9);

        System.out.println(isPrime(54161329) + " expect false");
        System.out.println(isPrime(1882341361) + " expect true");
        System.out.println(isPrime(2) + " expect true");
        int numPrimes = 0;

        for(int i = 2; i < 10000000; i++) {
            if(isPrime(i)) {
                numPrimes++;
            }
        }

        System.out.println(numPrimes + " " + System.currentTimeMillis());

        boolean[] primes = getPrimes(10000000);
        int np = 0;
        for(boolean b : primes)
            if(b)
                np++;

        System.out.println(np + " " + System.currentTimeMillis());

        System.out.println(new BigInteger(1024, 10, new Random()));
        int max=8;
        boolean[] result = new boolean[max + 1];
        for(int i = 2; i < result.length; i++)
            result[i] = true;
        final double LIMIT = Math.sqrt(max);
        for(int i = 2; i <= LIMIT; i++) {
            if(result[i]) {
                // cross out all multiples;
                int index = 2 * i;
                while(index < result.length){
                    result[index] = false;
                    index += i;
                }
            }
        }
        int num=4;
        int expectedFactors=5;
        int actualFactors = numFactors(num);

        System.out.println("Testing " + num + " expect " + expectedFactors + ", " +
                "actual " + actualFactors);
        if(actualFactors == expectedFactors)
            System.out.println("PASSED");
        else
            System.out.println("FAILED");
        System.out.println(System.currentTimeMillis());
    }

    public static boolean[] getPrimes(int max) {
        boolean[] result = new boolean[max + 1];
        for(int i = 2; i < result.length; i++)
            result[i] = true;
        final double LIMIT = Math.sqrt(max);
        for(int i = 2; i <= LIMIT; i++) {
            if(result[i]) {
                // cross out all multiples;
                int index = 2 * i;
                while(index < result.length){
                    result[index] = false;
                    index += i;
                }
            }
        }
        return result;
    }


    public static void printTest(int num, int expectedFactors) {

        int actualFactors = numFactors(num);

        System.out.println("Testing " + num + " expect " + expectedFactors + ", " +
                "actual " + actualFactors);
        if(actualFactors == expectedFactors)
            System.out.println("PASSED");
        else
            System.out.println("FAILED");
        System.out.println(System.currentTimeMillis());
    }

    // pre: num >= 2
    public static boolean isPrime(int num) {
        assert num >= 2 : "failed precondition. num must be >= 2. num: " + num;
        final double LIMIT = Math.sqrt(num);
        boolean isPrime = (num == 2) ? true : num % 2 != 0;
        int div = 3;
        while(div <= LIMIT && isPrime) {
            isPrime = num % div != 0;
            div += 2;
        }
        return isPrime;
    }

    // pre: num >= 2
    public static int numFactors(int num) {
        assert num >= 2 : "failed precondition. num must be >= 2. num: " + num;
        int result = 0;
        final double SQRT = Math.sqrt(num);
        for(int i = 1; i < SQRT; i++) {
            if(num % i == 0) {
                result += 2;
            }
        }
        if(num % SQRT == 0)
            result++;
        return result;
    }
}
