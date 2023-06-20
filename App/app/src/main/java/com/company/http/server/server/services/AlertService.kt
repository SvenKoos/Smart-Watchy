package com.company.http.server.server.services

import com.company.http.server.server.MissingParamsException
import com.company.http.server.server.model.Alert
import com.company.http.server.server.repositories.AlertRepository
import org.koin.core.component.KoinComponent
import org.koin.core.component.inject

class AlertService (): KoinComponent {

    private val alertRepository by inject<AlertRepository>()

    fun alertList(): List<Alert> = alertRepository.alertList()

    fun addAlert(alert: Alert): Alert {
        if (alert.id == 0)
            throw MissingParamsException("id")
        if (alert.appName == null)
            throw MissingParamsException("appName")
        if (alert.title == null)
            throw MissingParamsException("title")
        if (alert.body == null)
            throw MissingParamsException("body")
        if (alert.timestamp == null)
            throw MissingParamsException("timestamp")
        return alertRepository.addAlert(alert)
    }

    fun removeAlert(id: Int): Alert = alertRepository.removeAlert(id)

    fun removeAllAlerts() = alertRepository.removeAllAlerts()
}