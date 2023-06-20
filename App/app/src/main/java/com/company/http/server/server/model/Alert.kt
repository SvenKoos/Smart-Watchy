package com.company.http.server.server.model

data class Alert(
    val id: Int? = null,
    val appName: String? = null,
    val title: String? = null,
    val body: String? = null,
    val timestamp: String? = null,
    val dismissed: Boolean? = false,
)