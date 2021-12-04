**The Azure IoT SDKs team wants to hear from you!**

- [Need support?](#need-support)
- [Contribute code or documentation](#contribute-code-or-documentation)

# Need Support?
* Have a feature request for SDKs? Please post it on [User Voice](https://feedback.azure.com/forums/321918-azure-iot) to help us prioritize.
* Have a technical question? Ask on [Stack Overflow](https://stackoverflow.com/questions/tagged/azure-iot-hub) with tag "azure-iot-hub".
* Need Support? Every customer with an active Azure subscription has access to [support](https://docs.microsoft.com/azure/azure-supportability/how-to-create-azure-support-request) with guaranteed response time.  Consider submitting a ticket and get assistance from Microsoft support team
* Found a bug? Please help us fix it by thoroughly documenting it and filing an issue on GitHub ([C](https://github.com/Azure/azure-iot-sdk-c), [Java](https://github.com/Azure/azure-iot-sdk-java), [.NET](https://github.com/Azure/azure-iot-sdk-csharp), [Node.js](https://github.com/Azure/azure-iot-sdk-node), [Python](https://github.com/Azure/azure-iot-sdk-python)).


*Our SDKs are open-source and we do accept pull-requests if you feel like taking a stab at fixing the bug and maybe adding your name to our commit history :) Please mention any relevant issue number in the pull request description.* Please see [Contribute code](#contribute-code) below.

# Contribute code or documentation
We require pull-requests for code and documentation to be submitted against the `main` branch in order to review and run it in our gated build system. We try to maintain a high bar for code quality and maintainability, we insist on having tests associated with the code, and if necessary, additions/modifications to the requirement documents.

Also, have you signed the [Contribution License Agreement](https://cla.microsoft.com/) ([CLA](https://cla.microsoft.com/))? A friendly bot will remind you about it when you submit your pull-request.

**If your contribution is going to be a major effort, you should give us a heads-up first. We have a lot of items captured in our backlog and we release every two weeks, so before you spend the time, just check with us to make sure your plans and ours are in sync :) Just open an issue on github and tag it as "contribution".**

## Editing module requirements
We use requirement documents to describe the expected behavior for each code module. It works as a basis to understand what tests need to be written.

Each requirement has a unique tag that is re-used in the code comments to identify where it's implemented and where it's tested.

Each unique tag is in the following form:
SRS_<MODULE_NAME>_<DEVELOPER_ID>_<REQUIREMENT_ID>

When contributing to requirement docs, you can use `99` as a DEVELOPER_ID, and just increment the requirement ID to be unique.

For an example see the template in the [Adding new files](#adding-new-files)

## Adding new files
If your contribution is not part of an already existing code, you must create a new requirement file and a new unit test project. Our team created a template to help you on it. For the requirements you can copy the [template_requirements.md](https://github.com/Azure/azure-c-shared-utility/blob/master/devdoc/template_requirements.md) to the appropriate `devdoc` directory and change it to fits your needs.

For the unit test, copy the directory [template_ut](https://github.com/Azure/azure-c-shared-utility/tree/master/tests/template_ut) to the appropriate `tests` directory and change it to fits your needs. To include your new test suite in the `cmake`, add your new test suite in `test/CMakeLists.txt` with the command `add_subdirectory(template_ut)`.

## Contributing Ports
We encourage ports of our library to other platforms. Detailed process and guidelines around that process will be provided soon...

## Review Process
We expect all guidelines to be met before accepting a pull request. As such, we will work with you to address issues we find by leaving comments in your code. Please understand that it may take a few iterations before the code is accepted as we maintain high standards on code quality. Once we feel comfortable with a contribution, we will validate the change and accept the pull request.

Thank you for any contributions! Please let the team know if you have any questions or concerns about our contribution policy.
