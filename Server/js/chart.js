var thermostat = "beachwood";
var apiKey = "fc90b5ba-541b-43a6-a7c0-4c45bf14526d";

function RedrawGraph()
{

    $.getJSON('get-data-hour.php', {key: apiKey, thermostat: thermostat}, function(jsonData) 
    {		
        var xDivider = 30;			
        
        var maxTargetTemperatureCelsius = 30.0;
        var minTargetTemperatureCelsius = 20.0;
        
        var graphData = "";
        for(var i = 0; i < jsonData.length; i++)
        {
            var temp = (1 - (jsonData[i].celsius - minTargetTemperatureCelsius) / (maxTargetTemperatureCelsius - minTargetTemperatureCelsius)) * 100;
            graphData += (jsonData[i].offset/xDivider) + "," + temp + " ";        
        }
        graphData += "0,1000 " + (jsonData[0].offset/xDivider) + ",1000";
        
        var graphElement = document.getElementById("temperatureGraph");
        graphElement.setAttribute("points", graphData);

        // var animationElement = document.getElementById("graphAnimation");

        // if(animationElement == null )
        // {
        //     animationElement = document.createElement("animate");
        //     graphElement.appendChild(animationElement);
        //     animationElement.id = "graphAnimation";
        // }

        // animationElement.setAttribute("attributeName", "points");
        // animationElement.setAttribute("dur", "500ms");
        // animationElement.setAttribute("to", graphData);
        // animationElement.beginElement();
        
    }); 
}

var selectedElement = false;
var offset = false;

var edgeOffset = 50;

function makeDraggable(evt) 
{
    var svg = evt.target;

    svg.addEventListener('mousedown', startDrag);
    svg.addEventListener('mousemove', drag);
    svg.addEventListener('mouseup', endDrag);
    svg.addEventListener('mouseleave', endDrag);
    function startDrag(evt) 
    {
        if (evt.target.classList.contains('draggable')) {
            selectedElement = evt.target;

            offset = getMousePosition(evt);

            // Get all the transforms currently on this element
            var transforms = selectedElement.transform.baseVal;
            // Ensure the first transform is a translate transform
            if (transforms.length === 0 || transforms.getItem(0).type !== SVGTransform.SVG_TRANSFORM_TRANSLATE) 
            {
                // Create an transform that translates by (0, 0)
                var translate = svg.createSVGTransform();
                translate.setTranslate(0, 0);
                // Add the translation to the front of the transforms list
                selectedElement.transform.baseVal.insertItemBefore(translate, 0);
            }
            
             // Get initial translation amount
            transform = transforms.getItem(0);
            offset.x -= transform.matrix.e;
            offset.y -= transform.matrix.f;
          }    
    }

    function drag(evt) 
    {
        if (selectedElement) 
        {
            evt.preventDefault();
            var coord = getMousePosition(evt);
            //selectedElement.setAttributeNS(null, "cx", coord.x - offset.x);
            
            var y = coord.y - offset.y;
            
            y = Math.min(100, y);
            y = Math.max(0, y);

            evt.preventDefault();
            var coord = getMousePosition(evt);
            transform.setTranslate(0, y);    
            
            console.log(y);

            // selectedElement.setAttributeNS(null, "cy", y);

            // var temperatureValueElement = document.getElementById("temperatureValue");
            // temperatureValueElement.setAttributeNS(null, "y", y);

//            transform.setTranslate(coord.x - offset.x, coord.y - offset.y);

          //  var t = (y - edgeOffset) / (svg.viewBox.baseVal.height - (edgeOffset * 2));
            //temperatureValueElement.innerHTML = t;

            
        }
    }
    
    function endDrag(evt) 
    {
        selectedElement = null;
    }

    function getMousePosition(evt) 
    {
        var CTM = svg.getScreenCTM();
        return {
          x: (evt.clientX - CTM.e) / CTM.a,
          y: (evt.clientY - CTM.f) / CTM.d
        };
    }
  }

