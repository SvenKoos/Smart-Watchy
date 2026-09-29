package com.company.http.server.server.controllers

import com.company.esp32.alerts.app.SettingsActivity
import com.company.http.server.server.HttpService
import com.company.http.server.server.model.Alert
import com.company.http.server.server.model.ResponseBase
import com.company.http.server.server.services.AlertService
import io.ktor.application.*
import io.ktor.request.*
import io.ktor.response.*
import io.ktor.routing.*
import io.ktor.http.*
import org.koin.ktor.ext.inject
import timber.log.Timber

fun Route.alertController(_httpService: HttpService) {

    val alertService by inject<AlertService>()

    val httpService = _httpService

    get("/alert") {
        Timber.d("alertController {operation=GET /alert}")

        var responseCode = HttpStatusCode.OK
        val prefRemoteMACAddress = SettingsActivity.prefRemoteMACAddress

        if (prefRemoteMACAddress != "00:00:00:00:00:00") {
            // Sicheres Auslesen des URL-Query-Parameters "MAC"
            val remoteMACAddress = call.request.queryParameters["MAC"]?.replace("%3A", ":")

            Timber.d("alertController {operation=GET /alert}, prefRemoteMACAddress=${prefRemoteMACAddress}, remoteMACAddress=${remoteMACAddress}")

            if (remoteMACAddress == null || prefRemoteMACAddress != remoteMACAddress) {
                responseCode = HttpStatusCode.Forbidden
            }
        }

        if (responseCode == HttpStatusCode.OK) {
            httpService.alertService = alertService
            call.respond(responseCode, ResponseBase(data = alertService.alertList()))
        } else {
            call.respond(responseCode, ResponseBase(data = "Forbidden"))
        }
    }

    post("/alert") {
        val alert = call.receive<Alert>()
        Timber.d("alertController {operation=POST /alert}")
        call.respond(ResponseBase(data = alertService.addAlert(alert)))
    }

    delete("/alert/{id}") {
        val id = call.parameters["id"]?.toInt()!! // Force just for this example
        Timber.d("alertController {operation=DELETE /alert},id=${id}")
        call.respond(ResponseBase(data = alertService.removeAlert(id)))
    }
}