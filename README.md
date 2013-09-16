# Transform Flow Visualisation

Transform Flow Visualisation is a tool for offline analyse of mobile device sensor data and video streams. It is designed to be a toolbox for developing new algorithms for image alignment, localisation and mapping.

## Build and Install

Use [teapot][teapot] to build and install Transform Flow. You will need Ruby 1.9.3+ (preferably 2.0+) to install:

	$ gem install teapot

Once you've downloaded source code, build as follows:

	$ cd transform-flow-visualisation
	$ teapot fetch
	$ teapot build Library/TransformFlow variant-debug

To run Transform Flow visualisation with the included sample data:

	$ cd "./teapot/platforms/transform-flow/$PLATFORM/Applications/Transform Flow"
	$ ./transform-flow [path to dataset] BasicSensorMotionModel

Currently, only Mac OS X and Linux are supported using standards conformant C++11 compilers.

[teapot]: http://www.kyusu.org

## Stream Capture

The current stream capture tool is written for iOS and compiled using Xcode. You can find this project in `opt/Video Stream Capture-iOS`.

### Stream Format

The video stream format consists of a directory of images and a CSV log file.

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## License

Released under the MIT license.

Copyright, 2013, by [Samuel G. D. Williams](http://www.codeotaku.com/samuel-williams).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.