#pragma once

static const char PAGE_HEAD[] = R"html(<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="/favicon.ico" rel="icon" type="image/x-icon">
    <title>%TITLE%</title>
    <style>
        body { font-family: sans-serif; }
        td:first-child { padding-right: 1em; }
    </style>
</head>
<body>
)html";

static const char PAGE_FOOT[] = R"html(</body>
</html>
)html";
