import { CreatableMultiSelectControl } from "@arangodb/ui";
import { Grid } from "@chakra-ui/react";
import React from "react";
import { useAnalyzersContext } from "../../AnalyzersContext";

export const MultiDelimiterConfig = ({
  basePropertiesPath
}: {
  basePropertiesPath: string;
}) => {
  const { isFormDisabled: isDisabled } = useAnalyzersContext();

  return (
    <Grid templateColumns={"1fr 1fr"} columnGap="4">
      <CreatableMultiSelectControl
        isDisabled={isDisabled}
        name={`${basePropertiesPath}.delimiters`}
        label="Delimiters"
      />
    </Grid>
  );
};
