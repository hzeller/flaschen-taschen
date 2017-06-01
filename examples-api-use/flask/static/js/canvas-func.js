function init_canvas(w,h)
{

    var canvas = document.getElementById('ftCanvas');
    var ctx = canvas.getContext("2d");


    function animFunc() 
    {
        var d = new Date();
        var t = d.getTime();
        const speed = 0.01
        const centerX = 20 + Math.sin(t * speed) * 25
        const centerY = 20
        const radius = 15

        ctx.fillStyle = '#222222'
        ctx.clearRect(0, 0, w, h);

        ctx.beginPath();
        ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
        ctx.fillStyle = 'green';
        ctx.fill();
        ctx.lineWidth = 1;
        ctx.strokeStyle = '#003300';
        ctx.stroke();
        window.requestAnimationFrame(animFunc);
    }

    animFunc();
}
