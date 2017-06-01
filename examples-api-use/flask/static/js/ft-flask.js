function init_ft(w,h)
{

    var canvas = document.getElementById('ftCanvas');
 
    var intervalID = null

    var updateButton = document.getElementById("update")
    var formData = new FormData();
    var oReq = new XMLHttpRequest();

    updateButton.addEventListener("click", function () {
        if(intervalID)
        {
            window.clearInterval(intervalID)
            intervalID = null;
            updateButton.innerHTML = 'Start Updating Flaschen Taschen';
        }
        else
        {
            updateButton.innerHTML = 'Stop Updating Flaschen Taschen';

            intervalID = window.setInterval(function () {
                var d = new Date();
                var t0 = d.getTime();

                canvas.toBlob(function (blob) {

                    var uploadURL = "ProcessCanvasData";
                    formData.set("data", blob);
                    oReq.open("POST", uploadURL);
                    var d = new Date();
                    var t1 = d.getTime();

                    oReq.onload = function (oEvent) {

                        var d = new Date();
                        var t2 = d.getTime();
                        var totalTime = t2 - t0;
                        var blobTime = t1 - t0;
                        console.log("Uploaded canvas in " + totalTime + "ms ("+blob.size+" byte blob processed in "+blobTime+"ms)")
                    };

                    oReq.send(formData);
                }, "image/png", 0);
            }, 50);
        }

    });

}
