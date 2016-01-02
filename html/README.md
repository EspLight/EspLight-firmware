this folder includes all the html and css used
for the esplight firmware.
the cleancss folder contains the minified and cleaned css.

this source (cleancss/tidy.css) is converted to a byte array (cleancss/tidy_min_css.c) and used in
htmlCss.h

tool for converting the .css to .c used is: xxd (linux tool)
and is used as follows:
```
xxd -i tidy.css > tidy_min_css.c
```

for cleaning out unneeded css and javascript the tool used is grunt.
https://addyosmani.com/blog/removing-unused-css/

```json
uncss: {
  dist: {
    src: ['index.html'],
    dest: 'cleancss/tidy.css'
    options: {
      report: 'min' // optional: include to report savings
    }
  }
}

```
