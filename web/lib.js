function sendRequest(method, url, payload) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open(method, url, false); // false for synchronous request
    xmlHttp.withCredentials = true;
    xmlHttp.setRequestHeader('Content-Type', 'application/json')
    xmlHttp.send(payload);
    return xmlHttp.responseText;
}

mergeInto(LibraryManager.library, {
    getResponse: function() { return "Hello, World! From JavaScript library!" },

    getOverviewData: function(year) { },

    getContent: function (year, month, day) {
        return "Hurr durr some dummy content";
    }
});
