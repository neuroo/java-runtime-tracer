import java.lang.*;
import java.io.*;
import java.util.*;
import java.net.*;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Sample {

	public static String filterXSS(String input) {

		if (input == null)
			return null;

		// Stolen from http://code.google.com/p/catnip-xss-filter
		HashMap<Pattern, String> quotesHashMap = new HashMap<Pattern, String>();
		HashMap<Pattern, String> angleBracketsHashMap = new HashMap<Pattern, String>();
		HashMap<Pattern, String> javaScriptHashMap = new HashMap<Pattern, String>();

		quotesHashMap.put(Pattern.compile("\""), "&quot;");
		quotesHashMap.put(Pattern.compile("\'"), "&#39;");
		quotesHashMap.put(Pattern.compile("`"), "&#96;");
		angleBracketsHashMap.put(Pattern.compile("<"), "&lt;");
		angleBracketsHashMap.put(Pattern.compile(">"), "&gt;");
		javaScriptHashMap.put(Pattern.compile("<(\\s*)(/\\s*)?script(\\s*)>"), "<$2script-disabled>");
		javaScriptHashMap.put(Pattern.compile("%3Cscript%3E"), "%3Cscript-disabled%3E");
		javaScriptHashMap.put(Pattern.compile("alert(\\s*)\\("), "alert[");
		javaScriptHashMap.put(Pattern.compile("alert%28"), "alert%5B");
		javaScriptHashMap.put(Pattern.compile("document(.*)\\.(.*)cookie"), "document cookie");
		javaScriptHashMap.put(Pattern.compile("eval(\\s*)\\("), "eval[");
		javaScriptHashMap.put(Pattern.compile("setTimeout(\\s*)\\("), "setTimeout$1[");
		javaScriptHashMap.put(Pattern.compile("setInterval(\\s*)\\("), "setInterval$1[");
		javaScriptHashMap.put(Pattern.compile("execScript(\\s*)\\("), "execScript$1[");
		javaScriptHashMap.put(Pattern.compile("(?i)javascript(?-i):"), "javascript ");
		javaScriptHashMap.put(Pattern.compile("(?i)onclick(?-i)"), "oncl1ck");
		javaScriptHashMap.put(Pattern.compile("(?i)ondblclick(?-i)"), "ondblcl1ck");
		javaScriptHashMap.put(Pattern.compile("(?i)onmouseover(?-i)"), "onm0useover");
		javaScriptHashMap.put(Pattern.compile("(?i)onmousedown(?-i)"), "onm0usedown");
		javaScriptHashMap.put(Pattern.compile("(?i)onmouseup(?-i)"), "onm0useup");
		javaScriptHashMap.put(Pattern.compile("(?i)onmousemove(?-i)"), "onm0usemove");
		javaScriptHashMap.put(Pattern.compile("(?i)onmouseout(?-i)"), "onm0useout");
		javaScriptHashMap.put(Pattern.compile("(?i)onchange(?-i)"), "onchahge");
		javaScriptHashMap.put(Pattern.compile("(?i)onfocus(?-i)"), "onf0cus");
		javaScriptHashMap.put(Pattern.compile("(?i)onblur(?-i)"), "onb1ur");
		javaScriptHashMap.put(Pattern.compile("(?i)onkeypress(?-i)"), "onkeyqress");
		javaScriptHashMap.put(Pattern.compile("(?i)onkeyup(?-i)"), "onkeyuq");
		javaScriptHashMap.put(Pattern.compile("(?i)onkeydown(?-i)"), "onkeyd0wn");
		javaScriptHashMap.put(Pattern.compile("(?i)onload(?-i)"), "onl0ad");
		javaScriptHashMap.put(Pattern.compile("(?i)onreset(?-i)"), "onrezet");
		javaScriptHashMap.put(Pattern.compile("(?i)onselect(?-i)"), "onzelect");
		javaScriptHashMap.put(Pattern.compile("(?i)onsubmit(?-i)"), "onsubm1t");
		javaScriptHashMap.put(Pattern.compile("(?i)onunload(?-i)"), "onunl0ad");
		javaScriptHashMap.put(Pattern.compile("&#x61;&#x6C;&#x65;&#x72;&#x74;"), "a1ert");

		HashMap<Pattern, String> parameterEscapes = new HashMap<Pattern, String>();
		parameterEscapes.putAll(quotesHashMap);
		parameterEscapes.putAll(angleBracketsHashMap);
		parameterEscapes.putAll(javaScriptHashMap);

		String currentContent = input;
        // Loop through each of the substitution patterns.
        Iterator escapesIterator = parameterEscapes.keySet().iterator();
        while (escapesIterator.hasNext()) {
            Pattern pattern = (Pattern) escapesIterator.next();
            Matcher matcher = pattern.matcher(currentContent);
            currentContent = matcher.replaceAll((String)parameterEscapes.get(pattern));
        }

        return currentContent;
	}


	public static Integer testStuff(Integer i, Double d) {
		Integer n = new Integer(0);
		return n + i;
	}

	// Implementation of hibernate email validator
	public static boolean isValidEmail(String emailAddress) {
		String ATOM = "[^\\x00-\\x1F^\\(^\\)^\\<^\\>^\\@^\\,^\\;^\\:^\\\\^\\\"^\\.^\\[^\\]^\\s]";
		String DOMAIN = "(" + ATOM + "+(\\." + ATOM + "+)*";
		String IP_DOMAIN = "\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\]";
		Pattern pattern = Pattern.compile("^" + ATOM + "+(\\." + ATOM + "+)*@" + DOMAIN + "|" + IP_DOMAIN + ")$", 2);

	    if (emailAddress == null) return true;
	    if (!(emailAddress instanceof String)) return false;
	    String string = (String)emailAddress;
	    if (string.length() == 0) return true;
	    Matcher m = pattern.matcher(string);
	    return m.matches();
	}


	public static void foo(String test) {
		String test2 = "1";
		System.out.println(test + test2);
	}

	public static void main(String[] args) {
		foo("Class loaded...");
		filterXSS("<script>This is a test... ' <> or ");
		isValidEmail("foo[]#plop.com");
		testStuff(4, 4.42);
	}
}

