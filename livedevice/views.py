"""
mbed tools
Copyright (c) 2018 ARM Limited
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""


"""live-device view functions

These functions handle web requests, such as the
live-device page and mbed cloud webhooks.
"""
import json
import logging
import re

from django.views.decorators.csrf import csrf_exempt
from django.shortcuts import render
from django.http import (
    HttpResponse,
    HttpResponseNotAllowed,
    HttpResponseBadRequest,
    HttpResponseForbidden,
    JsonResponse
)

from channels import Group, Channel

from .models import WebHookAuth, Board
from .cloud_data_handlers import handle_data


logger = logging.getLogger(__name__)


def livedevice(request):
    """
    Handle live-device URL.

    Args:
        request: the request object
    """
    context = {}
    return render(request, "livedevice/livedevice.html", context)


def showcache(request):
    board_data = Board.objects.all_data()
    return JsonResponse(board_data)


@csrf_exempt
def mbed_cloud_webhook(request):
    """
    Handler for webhook calls directly from mbed cloud

    Args:
        request: the request object
    """
    if request.method != 'PUT':
        return HttpResponseNotAllowed(['PUT'])

    # check authorization
    try:
        authorization_header = request.META['HTTP_AUTHORIZATION']
    except KeyError:
        return HttpResponse(
            json.dumps({"error": "Bearer authorization header required"}),
            status=401,
            content_type="application/json"
        )
    match = re.match("^Bearer (.*)$", authorization_header)
    if match is None:
        return HttpResponseBadRequest(
            json.dumps({"error": "Bearer authorization header malformed: %s" % authorization_header}),
            content_type="application/json"
        )
    api_key = match.group(1)
    try:
        webhook_auth = WebHookAuth.objects.get(api_key=api_key)
    except WebHookAuth.DoesNotExist:
        return HttpResponseForbidden(
            json.dumps({"error": "Unauthorized"}),
            content_type="application/json"
        )

    # authorization looks good so let's parse the data
    try:
        data = json.load(request)
    except ValueError as error:
        return HttpResponseBadRequest(
            json.dumps({"error": unicode(error)}),
            content_type="application/json")
    logger.debug("data: %r", data)
    try:
        handle_data(webhook_auth, data)
    except ValueError as e:
        return HttpResponseBadRequest(
                json.dumps({"error": e.msg}),
                content_type="application/json")
    return HttpResponse()


def device_finder(request):
    """
    Handle live-device/find URL.

    Args:
        request: the request object
    """
    context = {}
    return render(request, "livedevice/finddevice.html", context)
