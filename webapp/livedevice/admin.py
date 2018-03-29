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


from django.contrib import admin
from django.contrib import sites
from django.contrib import messages
from django.contrib.admin.options import IS_POPUP_VAR
from django.utils.translation import ugettext as _
from django.utils.encoding import force_text
from django.contrib.admin.templatetags.admin_urls import add_preserved_filters
from django.http import HttpResponseRedirect

from .models import Board, Sensor, WebHookAuth, MBEDCloudAccount, SiteScheme


class SensorInline(admin.TabularInline):
    model = Sensor
    readonly_fields = ('value',)

    def value(self, sensor):
        return sensor.value


class BoardAdmin(admin.ModelAdmin):
    inlines = [
        SensorInline,
    ]

admin.site.register(Board, BoardAdmin)


class WebHookAuthInline(admin.TabularInline):
    model = WebHookAuth


class MBEDCloudAccountAdmin(admin.ModelAdmin):
    list_display = ('display_name', 'api_key', 'is_webhook_callback_set', 'is_long_polling')
    readonly_fields = ('webhook_callback_set', 'is_long_polling')
    search_fields = ('display_name', 'api_key')
    list_display_links = ('display_name', 'api_key')
    list_per_page = 5
    inlines = [
        WebHookAuthInline,
    ]

    def is_long_polling(self, account):
        return account.is_long_polling
    is_long_polling.boolean = True

    def response_change(self, request, account):
        if IS_POPUP_VAR not in request.POST:
            opts = self.model._meta
            pk_value = account._get_pk_val()
            preserved_filters = self.get_preserved_filters(request)

            msg_dict = {'name': force_text(opts.verbose_name), 'account': force_text(account)}
            if "_setwebhookcallback" in request.POST:
                try:
                    account.set_webhook_callback()
                except Exception as e:
                    msg_dict['error'] = str(e)
                    msg = _('The %(name)s "%(account)s" set webhook callback failed with "%(error)s."') % msg_dict
                    self.message_user(request, msg, messages.ERROR)
                else:
                    msg = _('The %(name)s "%(account)s" webhook callback has been set.') % msg_dict
                    self.message_user(request, msg, messages.SUCCESS)
                redirect_url = request.path
                redirect_url = add_preserved_filters({'preserved_filters': preserved_filters, 'opts': opts}, redirect_url)
                return HttpResponseRedirect(redirect_url)
            elif "_deletewebhookcallback" in request.POST:
                try:
                    account.delete_webhook_callback()
                except Exception as e:
                    msg_dict['error'] = str(e)
                    msg = _('The %(name)s "%(account)s" webhook callback delete failed with "%(error)s."') % msg_dict
                    self.message_user(request, msg, messages.ERROR)
                else:
                    msg = _('The %(name)s "%(account)s" webhook callback has been deleted.') % msg_dict
                    self.message_user(request, msg, messages.SUCCESS)
                redirect_url = request.path
                redirect_url = add_preserved_filters({'preserved_filters': preserved_filters, 'opts': opts}, redirect_url)
                return HttpResponseRedirect(redirect_url)
            elif "_startlongpoll" in request.POST:
                try:
                    account.start_long_polling()
                except Exception as e:
                    msg_dict['error'] = str(e)
                    msg = _('The %(name)s "%(account)s" start polling failed with %(error)s.') % msg_dict
                    self.message_user(request, msg, messages.ERROR)
                else:
                    msg = _('The %(name)s "%(account)s" polling has been started.') % msg_dict
                    self.message_user(request, msg, messages.SUCCESS)
                redirect_url = request.path
                redirect_url = add_preserved_filters({'preserved_filters': preserved_filters, 'opts': opts}, redirect_url)
                return HttpResponseRedirect(redirect_url)
            elif "_stoplongpoll" in request.POST:
                try:
                    account.stop_long_polling()
                except Exception as e:
                    msg_dict['error'] = str(e)
                    msg = _('The %(name)s "%(account)s" stop polling failed with %(error)s.') % msg_dict
                    self.message_user(request, msg, messages.ERROR)
                else:
                    msg = _('The %(name)s "%(account)s" polling is stopping.') % msg_dict
                    self.message_user(request, msg, messages.SUCCESS)
                redirect_url = request.path
                redirect_url = add_preserved_filters({'preserved_filters': preserved_filters, 'opts': opts}, redirect_url)
                return HttpResponseRedirect(redirect_url)
        return super(MBEDCloudAccountAdmin, self).response_change(request, account)

admin.site.register(MBEDCloudAccount, MBEDCloudAccountAdmin)


class SiteSchemeInline(admin.TabularInline):
    model = SiteScheme


class SiteAdmin(sites.admin.SiteAdmin):
    inlines = [SiteSchemeInline]
    list_display = ['domain', 'name', 'get_scheme']

    def get_scheme(self, obj):
        try:
            return obj.sitescheme.scheme
        except SiteScheme.DoesNotExist:
            return SiteScheme._meta.get_field('scheme').get_default()
    get_scheme.short_description = 'Scheme'

admin.site.unregister(sites.models.Site)
admin.site.register(sites.models.Site, SiteAdmin)


admin.site.register(WebHookAuth)
