/*
*  Author:
*  Rohit Muneshwar: 06 March 2022
* 
* This cpp file contains the translated cpp code from javascript code written in index_with_canvas.html file.
* Here is how it looks like.
*/

#include<emscripten/val.h>

using emscripten::val;

// Use thread_local when you want to retrieve & cache a global JS variable once per thread.
thread_local const val document = val::global("document");

int main() {
	// C-string val conversion is explicit in val.h so we need to pass val(***) like string as a second argument in below functino calls
	// my-canvas is the canvas created in a html file custom_hello_world_translated_javascript_with_embind.html and used here
	// Note: To use the emscripten generated html file, change the value my-canvas to canvas only.
	val canvas = document.call<val>("getElementById", val("my-canvas"));
	// Modify the casvas properties like below
	// canvas.set("width", val("800"));
	// canvas.set("height", val("600"));

	val ctx = canvas.call<val>("getContext", val("2d"));
	ctx.set("fillStyle", "green");
	ctx.call<void>("fillRect", 10, 10, 800, 600);
}