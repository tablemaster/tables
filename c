<table xmlns="http://query.yahooapis.com/v1/schema/table.xsd">
    <meta>
        <author>Originally by Sam Pullara, modified by me to allow dollar signs in names of a form's input fields </author>
        <description>Discover and process forms on a page</description>
        <sampleQuery>select * from {table} where url='http://www.javarants.com/'</sampleQuery>
        <sampleQuery>update {table} set input.s = 'yql' where url='http://www.javarants.com/'</sampleQuery>
        <sampleQuery>select * from form where input.s = 'yql' and url='http://www.javarants.com/' | xpath(pattern='//div[@class="searchresults"]/div[1]/div[1]')</sampleQuery>
    </meta>

    <execute><![CDATA[

// Manual adjustments:
// * Using (instead of request.headers.location as it is merely the absolute path
//   part of the full URL)
//    "http://www.cboe.com" + request.headers.location
// * Adding Referer header.
// * Form input fields with type "image" or "reset" are not included in the
//   submission via POST
// * Make all y.rest(...) calls time out after 15 seconds (15000 ms)

var DEBUG_IS_LIKE_ARRAY = false, DEBUG_COOKIES = false, DEBUG_REDIRECT = false;
var DEBUG_SUBMIT = false, DEBUG = false;
// When value=cookies, (typeof value === 'object') and (typeof value.length === 'number')
// are true; (typeof value.splice === 'function') and (value.constructor === Array) are
// false. The disabled "for" loop runs successfully. However, logging
//    !(value.propertyIsEnumerable('length'))
// fails with a catastrophic error "TypeError: Cannot find default value for object."
var isLikeArray = function (value) {
  if(DEBUG_IS_LIKE_ARRAY) {
    y.log("In isLikeArray:");
    y.log(value);
    y.log(typeof value === 'object');
    y.log(typeof value.length === 'number');
    y.log(typeof value.splice === 'function');
    y.log(value.constructor === Array);
    for (c in value) {
      y.log(c + ": " + value[c]);
    }
    
    y.log("Returning from isLikeArray");
  }
  
  return value &&
    typeof value === 'object' &&
    typeof value.length === 'number';
};

var cookie = "";
function getcookie(headers, cookie) {
  if(DEBUG_COOKIES) y.log("Redirect: 2a");
  var cookies = headers['set-cookie'];
  if(DEBUG_COOKIES) {
    y.log("Redirect: 2b");
    y.log(cookies);
    y.log(cookies instanceof Array); // false
    y.log(isLikeArray(cookies));
  }
  
  if (cookies instanceof Array || isLikeArray(cookies)) {
    if(DEBUG_COOKIES) y.log("Redirect: 2c");
    for (c in cookies) {
      cookie += cookies[c].split(';')[0] + "; "
    }
  } else {
    if(DEBUG_COOKIES) y.log("Redirect: 2d");
    if (cookies) {
      cookie = cookies.split(';')[0] + "; ";
    }
  }
  if(DEBUG_COOKIES) y.log("Redirect: 2e");
  return cookie;
}

function handleRedirect(request) {
  if(DEBUG_REDIRECT) { y.log("Redirect: 1"); y.log(request.url); }
  while (request.status == 301 || request.status == 302) {
    if(DEBUG_REDIRECT) { y.log("Redirect: 2"); y.log(request.status); y.log(request.headers); }
    
    cookie = getcookie(request.headers, cookie);
    if(DEBUG_REDIRECT) {
      y.log("Redirect: 3"); y.log(cookie);
      y.log("Redirect: 4"); y.log(request.url);
    }
    
    // request.headers.location turns out to be the absolute path
    // "/delayedQuote/QuoteData.dat"; the host is implied to be null, hence I
    // have to insert it in the front.
    request = y.rest("http://www.cboe.com" + request.headers.location)
      .timeout(15000).accept("text/html");
    request = request.header("Referer", "http://www.cboe.com/delayedQuote/QuoteTableDownload.aspx");
    if(DEBUG_REDIRECT) { y.log(request.url); y.log("Redirect: 5"); }
    
    if (cookie != "") {
      if(DEBUG_REDIRECT) y.log("Redirect: 6");
      request = request.header("Cookie", cookie);
    }
    
    if(DEBUG_REDIRECT) {
      y.log("Redirect: 7");
      y.log(request.url);
    }
    
    request = request.get();
    
    if(DEBUG_REDIRECT) {
      y.log(request.url); y.log("Redirect: 8");
      y.log("New status is:"); y.log(request.status);
    }
  }
  return request;
}

var request = y.rest(url).timeout(15000).followRedirects(false).accept("text/html").get();
request = handleRedirect(request);
var headers = request.headers;
if(DEBUG) y.log(request.response);
var result = request.response..form;
var forms = <result/>;
for (var f in result) {
    var form = result[f];
    var formname = form.@name;
    var action = form.@action;
    var method = form.@method;
    var inputs = form..input + form..textarea;
    forms.form +=
      <form action={action} name={formname} method={method}>
        {inputs}
      </form>
    ;
}
    ]]></execute>
    <bindings>
        <select>
            <inputs>
                <key id="url" paramType="variable" required="true"/>
                <key id="name" paramType="variable"/>
                <map id="input" paramType="variable" required="true"/>
            </inputs>
            <execute><![CDATA[
forms = forms.form;
var form = null;
if (name != null) {
  for (f in forms) {
    form = forms[f];
    if (form.@name == name) {
      break;
    } else {
      form = null;
    }
  }
} else {
  if (forms instanceof Array) {
    for (f in forms) {
      form = forms[f];
      break;
    }
  } else {
    form = forms;
  }
}
if (form == null) {
  response.object = <error>form not found</error>;
  y.exit();
}
var action = form.@action.toString();
if (!action.match(/^https?:\/\//)) {
  if (action.match(/^\//)) {
    action = url.match(/^(https?:\/\/[^\/]+)\/.*$/)[1] + action;
  } else {
    action = url.match(/^(.*\/)[^\/]*$/)[1] + action;
  }
}
var method = form.@method.toString().toLowerCase();

var params = "";
var submit = y.rest(action).timeout(15000).accept("text/html").followRedirects(false);
if (cookie != "") {
  submit = submit.header('Cookie', cookie);
}
submit = submit.header("Referer", "http://www.cboe.com/delayedQuote/QuoteTableDownload.aspx");
var length = form.input.length();
var firstParam = true;
for (i in form.input) {
  var inp = form.input[i];
  var type = inp.@type;
  var name = inp.@name;
  var value = inp.@value;
  if (type != "hidden" && type != "submit" && type != "reset") {
    if(DEBUG_SUBMIT) {
      y.log("input is:");
      y.log(input);
      var inputAsString = '';
      for (property in input) {
        inputAsString += property + ': ' + input[property]+'; ';
      }
      y.log("input as a string is:");
      y.log(inputAsString);
    
      y.log("Need value for name=\"" + name + "\"");
      y.log(name.toString().length);
    }
    
    if(name.toString().length == 0) {
      // y.log("Found an empty name; using an empty value.");
      // value = "";
      continue;
    } else {
      // Replace all occurrences of $ in the input field name the form expects
      // with underscores, so that the value of that input field can be set with
      // "input.name=value". As "name" is some sort of XML entity, need toString().
      var mangledName = name.toString().replace(/\$/g, "_");
      if(DEBUG_SUBMIT) y.log("Looking up mangled name \"" + mangledName + "\"");
      var inputvalue = input[ mangledName ];
      if(DEBUG_SUBMIT) y.log("Result: " + inputvalue);
      if (inputvalue) {
        value = inputvalue;
      }
    }
  }
  
  if(type != "image" && type != "reset") {
    var do_nothing;
  } else {
    continue;
  }
  
  if (method == "post") {
    if(DEBUG_SUBMIT) { y.log("Adding to params:"); y.log(name); y.log(value); }
    if(! firstParam) {
      params += "&";
    } else {
      firstParam = false;
    }
      
    params += encodeURIComponent(name) + "=" + encodeURIComponent(value);
  } else {
    submit = submit.query(name, value);
  }
}

if(DEBUG) y.log("About to do a POST or a GET");
if (method == "post") {
  if(DEBUG) { y.log("Sending a post with params:"); y.log(params); }
  submit = submit.contentType("application/x-www-form-urlencoded").post(params);
} else {
  if(DEBUG) { y.log("Performing a GET"); }
  submit = submit.get();
}

if(DEBUG) {
  y.log("Current headers:");
  y.log(submit.headers);
  y.log("About to handle redirect");
}

submit = handleRedirect(submit);
if(DEBUG) y.log("Creating the result.");
//response.object = submit.response;
var fileContent = y.xpath(submit.response, "//p");
response.object = <data>{fileContent.text()}</data>;
            ]]></execute>
        </select>
        <select itemPath="result.form">
            <inputs>
                <key id="url" paramType="variable" required="true"/>
            </inputs>
            <execute><![CDATA[
response.object = forms;
            ]]></execute>
        </select>
        <update>
            <inputs>
                <key id="url" paramType="variable" required="true"/>
                <key id="name" paramType="variable"/>
                <map id="input" paramType="variable"/>
            </inputs>
            <execute><![CDATA[
forms = forms.form;
var form = null;
if (name != null) {
  for (f in forms) {
    form = forms[f];
    if (form.@name == name) {
      break;
    } else {
      form = null;
    }
  }
} else {
  if (forms instanceof Array) {
    for (f in forms) {
      form = forms[f];
      break;
    }
  } else {
    form = forms;
  }
}
if (form == null) {
  response.object = <error>form not found</error>;
  y.exit();
}
var action = form.@action.toString();
if (!action.match(/^https?:\/\//)) {
  if (action.match(/^\//)) {
    action = url.match(/^(https?:\/\/[^\/]+)\/.*$/)[1] + action;
  } else {
    action = url.match(/^(.*\/)[^\/]*$/)[1] + action;
  }
}
var method = form.@method.toString().toLowerCase();
var params = "";
var submit = y.rest(action).accept("text/html").followRedirects(false);
if (cookie != "") {
  submit = submit.header('Cookie', cookie);
}
var length = form.input.length();
for (i in form.input) {
  var inp = form.input[i];
  var type = inp.@type;
  var name = inp.@name;
  var value = inp.@value;
  if (type != "hidden") {
    // Replace all occurrences of $ in the input field name the form expects
    // with underscores, so that the value of that input field can be set with
    // input.name=value
    var inputvalue = input[ name.toString().replace(/\$/g, "_") ];
    if (inputvalue) {
      value = inputvalue;
    }
  }
  if (method == "post") {
    params += encodeURIComponent(name) + "=" + encodeURIComponent(value) + (i == length - 1 ? "" : "&");
  } else {
    submit = submit.query(name, value);
  }
}
if (method == "post") {
  submit = submit.contentType("application/x-www-form-urlencoded").post(params);
} else {
  submit = submit.get();
}
submit = handleRedirect(submit);
response.object = submit.response;
            ]]></execute>
        </update>
    </bindings>
</table>
