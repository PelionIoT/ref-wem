/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * Dependencies
 */
const fs = require('fs');
const gulp  = require('gulp');
const handlebars = require('gulp-compile-handlebars');
const path = require('path');
const rename = require('gulp-rename');

/**
 * Module body / Expose
 */
module.exports = (manifestPath, scriptSourceTemplate, staticRoot) => {
  const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
  const handlebarOpts = {
            helpers: {
                assetPath: (path, context) => {
                    return [staticRoot, context.data.root[path]].join('/');
                }
            }
        };
  const outputFile = path.basename(scriptSourceTemplate).replace('.hbs', '.html');
  return gulp.src(scriptSourceTemplate)
      .pipe(handlebars(manifest, handlebarOpts))
      .pipe(rename(outputFile));
};
