package com.company.http.server.server.repositories

import com.company.http.server.server.GeneralException
import com.company.http.server.server.model.Alert

const val maxNoAlerts = 20

interface AlertRepository {
    fun alertList(): ArrayList<Alert>

    fun addAlert(alert: Alert): Alert

    fun removeAlert(id: Int): Alert

    fun removeAllAlerts()
}

class AlertRepositoryImp (): AlertRepository {
    private var idCount = 0
    private val alertList = ArrayList<Alert>()

    override fun alertList(): ArrayList<Alert> = alertList

    override fun addAlert(alert: Alert): Alert {
        if (alertList.size >= maxNoAlerts) {
            alertList.removeFirst()
        }
        val newAlert = alert.copy(id = ++idCount)
        alertList.add(newAlert)
        return newAlert
    }

    override fun removeAlert(id: Int): Alert {
        alertList.find { it.id == id }?.let {
            alertList.remove(it)
            return it
        }
        throw GeneralException("Cannot remove alert: $id")
    }

    override fun removeAllAlerts() {
        alertList.clear()
    }
}