

function init_canvas(w, h)
{
    


    var errMsgDiv = document.getElementById('errMsg');
    var canvas = document.getElementById('ftCanvas');
    var ctx = canvas.getContext("2d");
    var webgl = canvas.getContext("experimental-webgl");
    function animateFunction(func) {
        fn = function () {
            var d = new Date();
            var t = d.getTime();
            func(ctx, webgl, t);

            requestAnimationFrame(fn);

        };

        fn();

        return fn;
    }

    function animFunc(ctx,gl,t,w,h) 
    {
        const speed = 0.01
        const centerX = 20 + Math.sin(t * speed) * 25
        const centerY = 20
        const radius = 15


        ctx.beginPath();
        ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
        ctx.fillStyle = 'green';
        ctx.fill();
        ctx.lineWidth = 1;
        ctx.strokeStyle = '#003300';
        ctx.stroke();
    }


    function errFunc(ctx, gl, t, w, h) {
        ctx.fillStyle = '#FF0000'
        ctx.clearRect(0, 0, w, h);
    }

    var lastUpdate = 0
    const funcData = {cb:animFunc}

    var updateFuncInput = document.getElementById('updateFunc');

    updateFuncInput.addEventListener("input", () =>
    {
        var d = new Date();
        var t = d.getTime();
        if (t - lastUpdate < 500)
            return;

        lastUpdate = t;

        var source = updateFuncInput.value
        try{
            var inputUpdateFunc = new Function("ctx", "gl", "t", "w", "h", source)
            try{
                inputUpdateFunc(ctx,webgl, t)
            }
            catch (e) {
                funcData.cb = errFunc
                errMsgDiv.innerHTML = "Error in code:" + e.message
                return;
            }
            funcData.cb = inputUpdateFunc
            errMsgDiv.innerHTML = "Code is good!"
        }
        catch(e)
        {


            errMsgDiv.innerHTML = "Runtime error in code:" + e.message
            funcData.cb = errFunc
            return
        }

    }
    , false);


    animateFunction(
        (ctx, t) =>
        {
            ctx.fillStyle = '#222222'
            ctx.clearRect(0, 0, w, h);

            funcData.cb(ctx, webgl, t, w, h);
        });
}
