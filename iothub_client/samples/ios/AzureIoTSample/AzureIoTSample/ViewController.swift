// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

import UIKit

class ViewController: UIViewController, logTarget {
    
    func addLogEntry(_ logEntry: String!) {
        DispatchQueue.main.async {
            self.logContent += logEntry
            self.txtLogDisplay.text = self.logContent
        }
    }
    
    var logContent = ""
    
    @IBOutlet weak var txtLogDisplay: UITextView!
    @IBOutlet weak var btnExit: UIButton!
    
    @IBAction func exitClicked(_ sender: UIButton) {
        exit(0)
    }
    
    func start() {
        logContent = ""
        // Connect the sample to the txtLogDisplay and start its main.
        DispatchQueue.global(qos: .userInitiated).async {
            init_connector(self)
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        txtLogDisplay.isEditable = false
        start()
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

}

