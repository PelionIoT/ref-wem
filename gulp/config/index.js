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


const templatePath = 'wem/templates/';
const staticRoot = 'static/';
const staticSource = staticRoot + 'src/';
const staticBuild = staticRoot + '_build/';
const staticDist = staticRoot + 'dist/';
const npmRoot = 'node_modules/';


exports = module.exports = {
    staticUrlRoot: '/site_media/static',
    paths: {
        source: staticSource,
        build: staticBuild,
        dist: staticDist
    },
    watch: {
        styles: [
            staticSource + 'less/**/*.less'
        ],
        scripts: [
            staticSource + 'js/**/*.js'
        ]
    },
    templates: {
        destination: templatePath,
        manifestPath: staticBuild + 'manifest.json',
        scriptsTemplate: staticSource + 'hbs/_scripts.hbs',
        stylesTemplate: staticSource + 'hbs/_styles.hbs',
    },
    fonts: {
        sources: [
            npmRoot + 'font-awesome/fonts/**.*',
            npmRoot + 'bootstrap/fonts/**.*',
        ],
        dist: staticDist + 'fonts/'
    },
    styles: {
        source: staticSource + 'less/site.less',
        dist: staticBuild + 'css/',
        npmPaths: [
            npmRoot + 'bootstrap/less',
            npmRoot + 'font-awesome/less',
            npmRoot
        ]
    },
    scripts: {
        main: staticSource + 'js/site.js',
        source: [
            staticSource + 'js/**/*'
        ],
        dist: staticBuild + 'js/'
    },
    images: {
        sources: [
            staticSource + 'images/**.*'
        ],
        dist: staticDist + 'images/'
    },
    manifest: {
        source: [
            staticBuild + '**/*.css',
            staticBuild + '**/*.js'
        ]
    },
    test: {
        all: 'test/**/*.test.js',
        req: 'test/req/*.test.js',
        components: 'test/components/*.test.js'
      },
    xo: {
       source: [
         'tasks/**/*.js',
         staticSource + '**/*.js'
       ]
   },
   optimize: {
     css: {
       source: staticDist + 'css/*.css',
       options: {},
       dist: staticDist + 'css/'
     },
     js: {
       source: staticDist + 'js/*.js',
       options: {},
       dist: staticDist + 'js/'
     }
   }
};
