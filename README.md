## Workplace Environmental Monitor Webapp

This web application collects temperature, humidity, and light level sensor data from one or more [Workplace Environment Monitor](https://github.com/ARMmbed/ref-wem) devices and displays the data in live charts.  The application is written in Python using Django and Pinax.

### Getting started

Install the required tools if you are using Linux:

```
sudo apt-get install python python-pip python-dev libffi-dev libssl-dev virtualenvwrapper
```

Note: you'll need to restart your terminal for virtualenvwrapper to work

Create a virtualenv and use it:
```
mkvirtualenv ref-wem-webapp
```

Install the dependencies
```
pip install -r requirements.txt
```

Initialize your secret key:
```
export SECRET_KEY="$(python -c 'from django.core.management.utils import get_random_secret_key; print get_random_secret_key()')"
echo "export SECRET_KEY='$SECRET_KEY'" >> $VIRTUAL_ENV/bin/postactivate
```


[Get a Google Maps API key](https://developers.google.com/maps/documentation/javascript/get-api-key) and add it to the environment:
```
export GOOGLE_MAPS_API_KEY='[the key from the link above]'
echo "export GOOGLE_MAPS_API_KEY='$GOOGLE_MAPS_API_KEY'" >> $VIRTUAL_ENV/bin/postactivate
```

Initialize the database:
```
./manage.py migrate
./manage.py loaddata sites
```

Run the development server:
```
./manage.py runserver
```

Then, in a separate terminal:

```
workon ref-wem-webapp
./manage.py rundelay
```

Then you can visit [http://localhost:8000/live-device/](http://localhost:8000/live-device/).

### Testing

Install the test simulator dependencies:
`pip install -r requirements_test.txt`

To test with the simulator:

1. `./manage.py loaddata testauth`.
1. `python simulate_fake_device.py`.

To test with a real device:

1. Use serveo.net to forward your dev server port onto the internet: `ssh -R 80:localhost:8000 serveo.net`.
1. Add a superuser to Django: `./manage.py createsuperuser`.
1. Change your site domain in the [Django admin](http://localhost:8000/admin/sites/site/1/) from `localhost:8000` to [mysubdomain].serveo.net where mysubdomain is whatever the serveo SSH command output was. Also set the scheme to HTTPS.
1. Make sure your site is available at the serveo URL by loading it in a browser.
1. Then add your Mbed Cloud account to the [Django admin](http://localhost:8000/admin/livedevice/mbedcloudaccount/):
   * URL: [the default is fine].
   * API key: [your Mbed Cloud API key].
   * Display name: [your choice of name].
   * Click `Save and continue editing`.
   * Click either `Set webhook callback` or `Start long poll`.
1. `Connected details` says `connecting`.
1. Refresh the page.
1. `Connected details` says `success`. You can now see callbacks at https://[mysubdomain].serveo.net/live-device/mbed-cloud-webhook/.

### Geolocation

The `Find Device` page at `/live-device/find/` displays a world map where pins are dropped at the latitude/longitude that registered devices report. Hover over or click on a pin to display additional information about the corresponding device.

A circle representing the accuracy of the latitude/longitude measurement is displayed around each pin. The radius of the circle represents the accuracy. A smaller radius indicates higher accuracy, and a larger circle indicates lower accuracy.

There are two types of geolocation services supported: `user` and `auto`. The type is communicated through M2M resource ID `/3336/x/5750`, where `x` represents the geolocation service instance. The administrator of the device manually configures the `user` type, whereas `auto` is determined by dynamic means.

On the world map, a device drops a single pin. A priority order is used to determine which of the geolocation services to use for the device's pin.

#### Geo type display priority

1. `user`

    If an administrator manually specifies the location of a device, then a pin is dropped at the specified location.

2. `auto`

    A device may determine its location using an online service or other dynamic means. If the administrator has not manually configured the location and the device was successful at reporting automatic location information, a pin is dropped at the reported location.

If no geolocation information is known about a device, the map does not display a pin for that device.

### Deploying for production on a Linux server

We have tested this on the Debian 9 operating system on an Amazon EC2 machine.

1. Install required software packages:

   ```
   sudo apt-get update
   sudo apt-get install python python-pip python-dev libffi-dev libssl-dev
   sudo apt-get install apache2 libapache2-mod-wsgi redis-server
   ```

1. Install Python virtual environment libraries:

   ```
   sudo pip install --upgrade pip
   sudo pip install virtualenv
   sudo pip install virtualenvwrapper
   ```

1. For the Apache webserver, enable `wsgi`, `proxy` and `proxy_wtunnel`:

   ```
   sudo a2enmod wsgi
   sudo a2enmod proxy
   sudo a2enmod proxy_wstunnel
   ```

1. Create a user and password for this web application:

   ```
   sudo useradd -m wem
   sudo passwd wem
   ```

1. Become the wem user and clone the ref-wem-webapp project under the wem home directory:

   ```
   sudo su wem
   cd ~/
   git clone https://github.com/ARMmbed/ref-wem-webapp.git
   ```

1. The ref-wem-webapp directory should be writable by the `www-data` user:

   ```
   sudo chgrp www-data ref-wem-webapp/
   ```

   You've successfully set permissions when you see:

   ```
   /home/wem/ref-wem-webapp$ ls -lah
   total 84K
   drwxrwxr-x 8 wem www-data   4.0K Oct 23 18:01 .
   drwxr-xr-x 4 wem wem        4.0K Oct 23 18:01 ..
   drwxrwxr-x 4 wem wem        4.0K Oct 23 18:01 wem
   drwxrwxr-x 8 wem wem        4.0K Oct 23 18:01 .git
   -rw-rw-r-- 1 wem wem          47 Oct 23 18:01 .gitignore
   -rwxrwxr-x 1 wem wem         255 Oct 23 18:01 manage.py
   -rw-rw-r-- 1 wem wem        3.4K Oct 23 18:01 package.json
   -rw-rw-r-- 1 wem wem        8.3K Oct 23 18:01 README.md
   -rw-rw-r-- 1 wem wem         267 Oct 23 18:01 requirements.txt
   -rw-rw-r-- 1 wem wem         14K Oct 23 18:01 simulate_fake_device.py
   drwxrwxr-x 4 wem wem        4.0K Oct 23 18:01 static
   -rw-rw-r-- 1 wem wem          70 Oct 23 18:01 tox.ini
   ...
   ```

1. Copy and paste these lines to the wem user's `~/.profile`:

   ```
   export WORKON_HOME=$HOME/.virtualenvs
   export PROJECT_HOME=$HOME/Devel
   source /usr/local/bin/virtualenvwrapper.sh
   ```

1. Create a virtualenv as the wem user.

   ```
   cd ~
   source .profile
   mkvirtualenv wem
   ```

1. Install the Python `requirements.txt` within that virtualenv:

   ```
   (wem) $ cd ref-wem-webapp/
   (wem) $ pip install -r requirements.txt
   (wem) $ ./manage.py migrate
   (wem) $ ./manage.py loaddata sites
   (wem) $ ./manage.py collectstatic --noinput
   ```

1. Create an admin user for your Django application:

   ```
   ./manage.py createsuperuser
   ```

1. Exit the wem user to switch back to the Linux root user:

   ```
   exit
   ```

1. Create a symlink for the Apache configuration:

   ```
   sudo ln -s /home/wem/ref-wem-webapp/wem/apache2_production.conf /etc/apache2/conf-available/wem.conf
   ```

1. Write the following to the Apache2 site file `/etc/apache2/sites-available/wem.conf`:

   ```
    <VirtualHost *:80>

        ServerAdmin webmaster@localhost
        DocumentRoot /var/www/html

        ErrorLog ${APACHE_LOG_DIR}/error_wem.log
        CustomLog ${APACHE_LOG_DIR}/access_wem.log combined

        Include conf-available/wem.conf

    </VirtualHost>
   ```

1. Disable the Apache default site and enable this web application site:

   ```
   sudo a2dissite 000-default.conf
   sudo a2ensite wem.conf
   ```

1. Copy the system service files to `/lib/systemd/system/`:

   ```
   sudo cp /home/wem/ref-wem-webapp/wem/*.service /lib/systemd/system/
   ```

1. Enable and start those services:

   ```
   sudo systemctl enable wem-worker.service
   sudo systemctl enable daphne.service
   sudo systemctl start wem-worker.service
   sudo systemctl start daphne.service
   ```

#### Debugging server

1. To see the logs for Apache2 (Django Python logs):

   ```
   $ tail /var/log/apache2/error_wem.log
   [Fri Sep 29 23:05:07.740793 2017] [wsgi:error] [pid 6497:tid 139758941665024] [remote 172.31.13.101:59914] data: {u'sensor': u'/3301/0/5700', u'board': u'015ec57ec09200000000000100100145', u'value': 2770.0}
   [Fri Sep 29 23:05:12.437946 2017] [wsgi:error] [pid 6497:tid 139759059252992] [remote 172.31.13.101:59918] data: {u'sensor': u'/3301/0/5700', u'board': u'015ec57ec09200000000000100100145', u'value': 2728.0}
   [Fri Sep 29 23:05:17.145264 2017] [wsgi:error] [pid 6497:tid 139758941665024] [remote 172.31.13.101:59924] data: {u'sensor': u'/3301/0/5700', u'board': u'015ec57ec09200000000000100100145', u'value': 2713.0}
   ```

1. To see the Daphne logs:

   ```
   $ sudo journalctl -f -u daphne.service
   ```

1. To see worker logs:

   ```
   $ sudo journalctl -f -u wem-worker.service
   -- Logs begin at Fri 2017-09-29 17:45:55 UTC. --
   Sep 29 22:26:49 ip-172-31-40-154 python[17436]: cache.get(u'015ec98f09b60000000000010010012e,/3336/0/5750'): 'user'
   Sep 29 22:26:49 ip-172-31-40-154 python[17436]: cache.get(u'015ec9dbc5f5000000000001001001a1,/26241/0/1'): 'device-1a1'
   Sep 29 22:26:49 ip-172-31-40-154 python[17436]: cache.get(u'015ec9dbc5f5000000000001001001a1,/3336/0/5750'): 'user'
   Sep 29 22:26:49 ip-172-31-40-154 python[17436]: cache.get(u'015ec9dbc5f5000000000001001001a1,/3304/0/5700'): 38.0
   ```

1. Any time you change code, do the following to restart the services:

   ```
   sudo systemctl reload apache2.service
   sudo systemctl restart daphne.service
   sudo systemctl restart wem-worker.service
   ```

### Adding sensors

To add your own sensors to the web application, you need the following 3 items:
* Resource name
* Resource URI (Find in the OMA LwM2M registry)
* Resource sensor range

There are 2 files that need to be changed to add the sensor to the web application.

The first file is `wem/settings/defaults.py`. Find the data struct at the very bottom of the file that looks like the following:

```
MBED_CLOUD_PRESUBSCRIPTIONS = [
        {"resource-path": ["/3301/0/*"]},
        {"resource-path": ["/3303/0/*"]},
        {"resource-path": ["/3304/0/*"]},
        {"resource-path": ["/3336/*"]},
        {"resource-path": ["/26241/0/*"]},
        {"resource-path": ["/26242/0/*"]}
]
```

Add another item into that `MBED_CLOUD_PRESUBSCRIPTIONS` struct that follows the following template:

`{"resource-path": ["/XXXX/0/*"]}`

Where XXXX is the Object ID of your resource.

The second file that needs to be modified is `livedevice/templates/livedevice/livedevice.html`. Find the data struct around line 39 that looks like the following:

```
    var chartsConfig = {
      '/3303/0/5700': {
        title: 'Temperature',
        element: null,
        chart: null,
        colors: palettes[palette_choice],
        min: 20,
        max: 30,
        label: "degrees C"
      },
      '/3301/0/5700': {
        title: 'Light',
        element: null,
        chart: null,
        colors: palettes[palette_choice],
        min: 0,
        max: 10000,
        label: "intensity (lux)"
      },
      '/3304/0/5700': {
        title: 'Humidity',
        element: null,
        chart: null,
        colors: palettes[palette_choice],
        min: 0,
        max: 100,
        label: "relative humidity(RH)"
      }
    }
```

Add another item into that `chartsConfig` struct that follows the following template:

```
'/XXXX/0/YYYY': {
   title: 'Sensor Title',
   element: null,
   chart: null,
   colors: palettes[palette_choice],
   min: 0,
   max: 10000,
   label: "Sensor units (unit)"
}
```

Where XXXX is the Object ID of your resource, and YYYY is the Resource ID of your resource.

That is it! Your web application should now should additional sensor information.
